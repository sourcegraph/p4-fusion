/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "change_list.h"

#include <utility>

#include "p4_api.h"
#include "describe_result.h"
#include "filelog_result.h"
#include "print_result.h"
#include "utils/std_helpers.h"
#include "minitrace.h"

ChangeList::ChangeList(const int& clNumber, std::string&& clDescription, std::string&& userID, const int64_t& clTimestamp)
    : number(clNumber)
    , user(std::move(userID))
    , description(std::move(clDescription))
    , timestamp(clTimestamp)
    , changedFileGroups(ChangedFileGroups::Empty())
{
}

void ChangeList::PrepareDownload(P4API& p4, GitAPI& git, const BranchSet& branchSet)
{
	MTR_SCOPE("ChangeList", __func__);

	if (branchSet.HasMergeableBranch())
	{
		// If we care about branches, we need to run filelog to get where the file came from.
		// Note that the filelog won't include the source changelist, but
		// that doesn't give us too much information; even a full branch
		// copy will have the target files listing the from-file with
		// different changelists than the point-in-time source branch's
		// changelist.
		const FileLogResult& filelog = p4.FileLog(git, number);
		if (filelog.HasError())
		{
			throw std::runtime_error(filelog.PrintError());
		}
		changedFileGroups = branchSet.ParseAffectedFiles(filelog.GetFileData());
	}
	else
	{
		// If we don't care about branches, then p4->Describe is much faster.
		const DescribeResult& describe = p4.Describe(git, number);
		if (describe.HasError())
		{
			ERR("Failed to describe changelist: " << describe.PrintError())
			throw std::runtime_error(describe.PrintError());
		}
		changedFileGroups = branchSet.ParseAffectedFiles(describe.GetFileData());
	}

	{
		std::unique_lock<std::mutex> lock(*downloadPreparedMutex);
		*downloadPrepared = true;
		downloadPreparedCV->notify_all();
	}
}

void ChangeList::StartDownload(P4API& p4, const int& printBatch)
{
	MTR_SCOPE("ChangeList", __func__);

	// wait for prepare to be finished.

	{
		std::unique_lock<std::mutex> lock(*downloadPreparedMutex);
		downloadPreparedCV->wait(lock, [this]()
		    { return downloadPrepared->load(); });
	}

	std::vector<FileData*> printBatchFileData;
	// Only perform the group inspection if there are files.
	if (changedFileGroups->totalFileCount > 0)
	{
		for (auto& branchedFileGroup : changedFileGroups->branchedFileGroups)
		{
			// Note: the files at this point have already been filtered.
			for (auto& fileData : branchedFileGroup.files)
			{
				if (fileData.IsDownloadNeeded())
				{
					fileData.SetPendingDownload();
					printBatchFileData.push_back(&fileData);

					// Clear the batches if it fits
					if (printBatchFileData.size() >= printBatch)
					{
						Flush(p4, printBatchFileData);

						// We let go of the refs held by us and create new ones to queue the next batch
						printBatchFileData.clear();
						// Now only the thread job has access to the older batch
					}
				}
			}
		}
	}

	// Flush any remaining files that were smaller in number than the total batch size.
	// Additionally, signal the batch processing end.
	Flush(p4, printBatchFileData);
	*downloadJobsCompleted = true;
	commitCV->notify_all();
}

class ChangeListPrintResultIterator : public PrintResult::PrintResultIterator
{
private:
	const std::vector<FileData*>& printBatchFileData;
	// Idx keeps track of the files index in printBatchFileData and is increased
	// every time we are told about a new file.
	// The PrintFiles function takes two callbacks, one for stat, basically "a new
	// file begins here", and then for small chunks of data of that file.
	long idx;

public:
	ChangeListPrintResultIterator(const std::vector<FileData*>& _printBatchFileData)
	    : printBatchFileData(_printBatchFileData),
		idx(-1)
	{
	}
	~ChangeListPrintResultIterator()
	{
		// If we have seen at least one file, we have to flush the last open file
		// to the ODB still, so let's do that.
		if (idx > -1)
		{
			printBatchFileData.back()->Finalize();
		}
	}

	void OnStat() override
	{
		// For the first file, we don't need to run finalize on the previous
		// file so we're done here.
		if (idx == -1)
		{
			idx++;
			printBatchFileData.at(idx)->StartWrite();
			return;
		}
		// First, finalize the previous file.
		printBatchFileData.at(idx)->Finalize();
		// Now step one file further.
		idx++;
		// And start a write for the next file.
		printBatchFileData.at(idx)->StartWrite();
	}
	void OnOutput(const char* contents, int length) override
	{
		// Write a chunk of the data to the currently processed file.
		printBatchFileData.at(idx)->Write(contents, length);
	}
};

void ChangeList::Flush(P4API& p4, const std::vector<FileData*>& printBatchFileData)
{
	MTR_SCOPE("ChangeList", __func__);

	// Only perform the batch processing when there are files to process.
	if (printBatchFileData.empty())
	{
		return;
	}

	std::vector<std::string> fileRevisions;
	fileRevisions.reserve(printBatchFileData.size());
	for (auto fileData : printBatchFileData)
	{
		std::string fileSpec = fileData->GetDepotFile();
		fileSpec.append("#");
		fileSpec.append(fileData->GetRevision());
		fileRevisions.push_back(fileSpec);
	}

	// Now we write the files that PrintFiles will give us to the git ODB in a
	// streaming fashion.
	PrintResult printResp = p4.PrintFiles(fileRevisions, ChangeListPrintResultIterator(printBatchFileData));
	if (printResp.HasError())
	{
		throw std::runtime_error(printResp.PrintError());
	}
}

void ChangeList::WaitForDownload()
{
	MTR_SCOPE("ChangeList", __func__);

	std::unique_lock<std::mutex> lock(*commitMutex);
	commitCV->wait(lock, [this]()
	    { return downloadJobsCompleted->load(); });
}

void ChangeList::Clear()
{
	user.clear();
	description.clear();
	changedFileGroups->Clear();

	commitMutex.reset();
	commitCV.reset();
}
