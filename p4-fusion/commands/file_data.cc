/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "file_data.h"
#include "git_api.h"

FileDataStore::FileDataStore()
    : actionCategory(FileAction::FileAdd)
    , isBlobOIDSet(false)
    , isContentsPendingDownload(false)
    , isBinary(false)
    , isExecutable(false)
{
}

FileData::FileData(GitAPI& git, std::string& depotFile, std::string& revision, std::string& action, std::string& type)
    : m_data(std::make_shared<FileDataStore>())
    , m_Git(git)
    , writer(nullptr)
{
	m_data->depotFile = depotFile;
	m_data->revision = revision;
	m_data->SetAction(action);
	m_data->isBinary = STDHelpers::Contains(type, "binary");
	m_data->isExecutable = STDHelpers::Contains(type, "+x");
	m_data->isBlobOIDSet = false;
	m_data->isContentsPendingDownload = false;
}

FileData::~FileData()
{
	free(writer);
}

FileData::FileData(const FileData& copy)
    : m_data(copy.m_data)
    , m_Git(copy.m_Git)
    , writer(nullptr)
{
}

FileData& FileData::operator=(FileData& other)
{
	if (this == &other)
	{
		// guard...
		return *this;
	}
	m_data = other.m_data;
	m_Git = other.m_Git;
	return *this;
}

void FileData::SetFromDepotFile(const std::string& fromDepotFile, const std::string& fromRevision)
{
	m_data->fromDepotFile = fromDepotFile;
	if (STDHelpers::StartsWith(fromRevision, "#"))
	{
		m_data->fromRevision = fromRevision.substr(1);
	}
	else
	{
		m_data->fromRevision = fromRevision;
	}
}

void FileData::StartWrite()
{
	if (m_data->isBlobOIDSet)
	{
		// Do not set the contents.  Assume that
		// they were already set or, worst case, are currently being set.
		return;
	}

	writer = m_Git.WriteBlob();
}

void FileData::Write(const char* contents, int length)
{
	writer->Write(contents, length);
}

void FileData::Finalize()
{
	m_data->blobOID = writer->Close();
	free(writer);
	writer = nullptr;
	m_data->isBlobOIDSet = true;
	m_data->isContentsPendingDownload = false;
}

void FileData::SetPendingDownload()
{
	if (!m_data->isBlobOIDSet)
	{
		m_data->isContentsPendingDownload = true;
	}
}

void FileData::SetRelativePath(std::string& relativePath)
{
	m_data->relativePath = relativePath;
}

FileAction extrapolateFileAction(std::string& action);

void FileDataStore::SetAction(std::string fileAction)
{
	actionCategory = extrapolateFileAction(fileAction);
	switch (actionCategory)
	{
	case FileAction::FileBranch:
	case FileAction::FileMoveAdd:
	case FileAction::FileIntegrate:
	case FileAction::FileImport:
		isIntegrated = true;
		isDeleted = false;
		break;

	case FileAction::FileDelete:
	case FileAction::FileMoveDelete:
	case FileAction::FilePurge:
		// Note: not including FileAction::FileArchive
		isDeleted = true;
		isIntegrated = false;
		break;

	case FileAction::FileIntegrateDelete:
		// This is the source of the integration,
		//   so even though this causes a delete to happen,
		//   as a source, there isn't something merging into this
		//   change.
		isIntegrated = false;
		isDeleted = true;
		break;

	default:
		isIntegrated = false;
		isDeleted = false;
	}
}

void FileDataStore::Clear()
{
	depotFile.clear();
	revision.clear();
	fromDepotFile.clear();
	fromRevision.clear();
	blobOID.clear();
	relativePath.clear();
}

FileAction extrapolateFileAction(std::string& action)
{
	if ("add" == action)
	{
		return FileAction::FileAdd;
	}
	if ("edit" == action)
	{
		return FileAction::FileEdit;
	}
	if ("delete" == action)
	{
		return FileAction::FileDelete;
	}
	if ("branch" == action)
	{
		return FileAction::FileBranch;
	}
	if ("move/add" == action)
	{
		return FileAction::FileMoveAdd;
	}
	if ("move/delete" == action)
	{
		return FileAction::FileMoveDelete;
	}
	if ("integrate" == action)
	{
		return FileAction::FileIntegrate;
	}
	if ("import" == action)
	{
		return FileAction::FileImport;
	}
	if ("purge" == action)
	{
		return FileAction::FilePurge;
	}
	if ("archive" == action)
	{
		return FileAction::FileArchive;
	}
	if (FAKE_INTEGRATION_DELETE_ACTION_NAME == action)
	{
		return FileAction::FileIntegrateDelete;
	}

	// That's all the actions known at the time of writing.
	// An unknown type, probably some future Perforce version with a new kind of action.
	if (STDHelpers::Contains(action, "delete"))
	{
		// Looks like a delete.
		WARN("Found an unsupported action " << action << "; assuming delete")
		return FileAction::FileDelete;
	}
	if (STDHelpers::Contains(action, "move/"))
	{
		// Looks like a new kind of integrate.
		WARN("Found an unsupported action " << action << "; assuming move/add")
		return FileAction::FileMoveAdd;
	}

	// assume an edit, as it's the safe bet.
	WARN("Found an unsupported action " << action << "; assuming edit")
	return FileAction::FileEdit;
}
