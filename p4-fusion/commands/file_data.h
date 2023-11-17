/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include <memory>
#include <atomic>
#include "common.h"
#include "utils/std_helpers.h"

class GitAPI;
class BlobWriter;

#define FAKE_INTEGRATION_DELETE_ACTION_NAME "FAKE merge delete"

// See https://www.perforce.com/manuals/cmdref/Content/CmdRef/p4_fstat.html
// for a list of actions.
enum FileAction
{
	FileAdd, // add
	FileEdit, // edit
	FileDelete, // delete
	FileBranch, // branch
	FileMoveAdd, // move/add
	FileMoveDelete, // move/delete
	FileIntegrate, // integrate
	FileImport, // import
	FilePurge, // purge
	FileArchive, // archive

	FileIntegrateDelete, // artificial action to reflect an integration that happened that caused a delete
};

struct FileDataStore
{
	// describe/filelog values
	std::string depotFile;
	std::string revision;
	bool isBinary;
	bool isExecutable;

	// filelog values
	//   - empty if not an integration style change
	std::string fromDepotFile;
	std::string fromRevision;

	// git blob data
	std::string blobOID;
	std::atomic<bool> isBlobOIDSet;
	std::atomic<bool> isContentsPendingDownload;

	// Derived Values
	std::string relativePath;
	FileAction actionCategory;
	bool isDeleted {};
	bool isIntegrated {}; // ... or copied, or moved, or ...

	FileDataStore() = delete;
	FileDataStore(std::string& _depotFile, std::string& _revision, std::string& action, std::string& type);

	void SetAction(std::string action);

	void Clear();
};

// For memory efficiency; the underlying data is passed around a bunch.
struct FileData
{
private:
	std::shared_ptr<FileDataStore> m_data;
	GitAPI& m_Git;
	BlobWriter* writer;

public:
	FileData() = delete;
	~FileData();
	FileData(GitAPI& git, std::string& depotFile, std::string& revision, std::string& action, std::string& type);
	FileData(const FileData& copy);
	FileData& operator=(FileData& other);

	void SetFromDepotFile(const std::string& fromDepotFile, const std::string& fromRevision);
	void SetRelativePath(std::string& relativePath);
	void SetFakeIntegrationDeleteAction() { m_data->SetAction(FAKE_INTEGRATION_DELETE_ACTION_NAME); };

	void StartWrite();
	void Write(const char* contents, int length);
	void Finalize();
	void SetPendingDownload();
	bool IsDownloadNeeded() const { return !m_data->isBlobOIDSet && !m_data->isContentsPendingDownload; };

	const std::string& GetDepotFile() const { return m_data->depotFile; };
	const std::string& GetRevision() const { return m_data->revision; };
	const std::string& GetRelativePath() const { return m_data->relativePath; };
	const std::string& GetBlobOID() const
	{
		if (!m_data->isBlobOIDSet)
		{
			throw std::runtime_error("Tried to access blob OID before it was set");
		}
		return m_data->blobOID;
	};
	bool IsDeleted() const { return m_data->isDeleted; };
	bool IsIntegrated() const { return m_data->isIntegrated; };
	const std::string& GetFromDepotFile() const { return m_data->fromDepotFile; };

	bool IsBinary() const { return m_data->isBinary; };
	bool IsExecutable() const { return m_data->isExecutable; };

	void Clear() { m_data->Clear(); };
};
