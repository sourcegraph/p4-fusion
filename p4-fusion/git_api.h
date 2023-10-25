/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include <string>
#include <vector>
#include <utility>

#include "common.h"
#include "commands/file_data.h"
#include "commands/change_list.h"
#include "git2/oid.h"

#define GIT2(x)                                                                \
	do                                                                         \
	{                                                                          \
		int error = x;                                                         \
		if (error < 0)                                                         \
		{                                                                      \
			const git_error* e = git_error_last();                             \
			ERR("GitAPI: " << error << ":" << e->klass << ": " << e->message); \
			exit(error);                                                       \
		}                                                                      \
	} while (false)

struct git_repository;

class BlobWriter
{
private:
	git_repository* repo;
	git_writestream* writer;
	bool begun;
	bool finalized;

public:
	BlobWriter() = delete;
	BlobWriter(git_repository* repo);

	void Write(const char* contents, int length);
	std::string Close();
};

class GitAPI
{
	git_repository* m_Repo = nullptr;
	git_oid m_FirstCommitOid;
	std::string repoPath;
	int timezoneMinutes;

public:
	GitAPI(const std::string& repoPath, bool fsyncEnable, int timezoneMinutes);
	GitAPI() = delete;
	~GitAPI();

	BlobWriter* WriteBlob() const;

	void InitializeRepository(bool noCreateBaseCommit);
	void OpenRepository();
	bool IsHEADExists() const;
	bool IsRepositoryClonedFrom(const std::string& depotPath) const;
	/* Checks if a previous commit was made and extracts the corresponding changelist number. */
	const std::string DetectLatestCL() const;

	/* files are cleared as they are visited. Empty targetBranch means HEAD. */
	std::string WriteChangelistBranch(
	    const std::string& depotPath,
	    const ChangeList& cl,
	    std::vector<FileData>& files,
	    const std::string& targetBranch,
	    const std::string& authorName,
	    const std::string& authorEmail,
	    const std::string& mergeFrom) const;
};
