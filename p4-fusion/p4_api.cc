/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "p4_api.h"

#include <csignal>

#include "utils/std_helpers.h"

#include "p4/p4libs.h"
#include "p4/signaler.h"
#include "minitrace.h"

ClientResult::ClientSpecData P4API::ClientSpec;
std::string P4API::P4PORT;
std::string P4API::P4USER;
std::string P4API::P4CLIENT;
int P4API::CommandRetries = 1;
int P4API::CommandRefreshThreshold = 1;

P4API::P4API()
{
	if (!Initialize())
	{
		ERR("Could not initialize P4API")
		throw std::runtime_error("Could not initialize P4API");
	}

	AddClientSpecView(ClientSpec.mapping);
}

bool P4API::Initialize()
{
	MTR_SCOPE("P4", __func__);

	Error e;
	StrBuf msg;

	m_Usage = 0;
	m_ClientAPI.SetPort(P4PORT.c_str());
	m_ClientAPI.SetUser(P4USER.c_str());
	m_ClientAPI.SetClient(P4CLIENT.c_str());
	m_ClientAPI.SetProtocol("tag", "");
	m_ClientAPI.Init(&e);

	if (!CheckErrors(e, msg))
	{
		ERR("Could not initialize Helix Core C/C++ API")
		return false;
	}

	return true;
}

bool P4API::Deinitialize()
{
	Error e;
	StrBuf msg;

	m_ClientAPI.Final(&e);
	return CheckErrors(e, msg);
}

bool P4API::Reinitialize()
{
	MTR_SCOPE("P4", __func__);

	bool status = Deinitialize() && Initialize();
	return status;
}

P4API::~P4API()
{
	if (!Deinitialize())
	{
		ERR("P4API context was not destroyed successfully");
	}
}

bool P4API::IsDepotPathValid(const std::string& depotPath)
{
	return STDHelpers::EndsWith(depotPath, "/...") && STDHelpers::StartsWith(depotPath, "//");
}

bool P4API::IsDepotPathUnderClientSpec(const std::string& depotPath)
{
	return m_ClientMapping.IsInLeft(depotPath);
}

bool P4API::CheckErrors(Error& e, StrBuf& msg)
{
	if (e.Test())
	{
		e.Fmt(&msg);
		ERR(msg.Text());
		return false;
	}
	return true;
}

bool P4API::InitializeLibraries()
{
	Error e;
	P4Libraries::Initialize(P4LIBRARIES_INIT_ALL, &e);
	if (e.Test())
	{
		StrBuf msg;
		e.Fmt(&msg);
		ERR(msg.Text());
		ERR("Failed to initialize P4Libraries");
		return false;
	}

	// We disable the default signaler to stop it from deleting memory from the wrong heap
	// https://www.perforce.com/manuals/p4api/Content/P4API/chapter.clientprogramming.signaler.html
	std::signal(SIGINT, SIG_DFL);
	signaler.Disable();

	SUCCESS("Initialized P4Libraries successfully");
	return true;
}

bool P4API::ShutdownLibraries()
{
	Error e;
	P4Libraries::Shutdown(P4LIBRARIES_INIT_ALL, &e);
	if (e.Test())
	{
		StrBuf msg;
		e.Fmt(&msg);
		ERR(msg.Text())
		return false;
	}
	return true;
}

void P4API::AddClientSpecView(const std::vector<std::string>& viewStrings)
{
	m_ClientMapping.InsertTranslationMapping(viewStrings);
}

ClientResult P4API::Client()
{
	return Run<ClientResult>("client", { "-o" });
}

TestResult P4API::TestConnection(const int retries)
{
	return RunEx<TestResult>("changes", { "-m", "1", "//..." }, retries);
}

ChangesResult P4API::Changes(const std::string& path, const std::string& from, int32_t maxCount)
{
	std::vector<std::string> args = {
		"-l", // Get full descriptions instead of sending cut-short ones
		"-s", "submitted", // Only include submitted CLs
		"-r" // Send CLs in chronological order
	};

	// This needs to be declared outside the if scope below to
	// keep the internal character array alive till the p4 call is made
	std::string maxCountStr;
	if (maxCount != -1)
	{
		maxCountStr = std::to_string(maxCount);

		args.emplace_back("-m"); // Only send max this many number of CLs
		args.push_back(maxCountStr);
	}

	std::string pathAddition;
	if (!from.empty())
	{
		// Appending "@CL_NUMBER,@now" seems to include the current CL,
		// which makes this awkward to deal with in general. So instead,
		// we append "@>CL_NUMBER" so that we only receive the CLs after
		// the current one.
		pathAddition = "@>" + from;
	}

	args.push_back(path + pathAddition);

	ChangesResult result = Run<ChangesResult>("changes", args);

	return result;
}

DescribeResult P4API::Describe(const int cl)
{
	MTR_SCOPE("P4", __func__);

	return Run<DescribeResult>("describe", { "-s", // Omit the diffs
	                                           std::to_string(cl) });
}

FileLogResult P4API::FileLog(const int changelist)
{
	return Run<FileLogResult>("filelog", {
	                                         "-c", // restrict output to a single changelist
	                                         std::to_string(changelist),
	                                         "-m1", // don't get the full history, just the first entry.
	                                         "//..." // rather than require the path to be passed in, just list all files.
	                                     });
}

PrintResult P4API::PrintFiles(const std::vector<std::string>& fileRevisions, const std::function<void()>& onStat, const std::function<void(const char*, int)>& onOutput)
{
	MTR_SCOPE("P4", __func__);

	if (fileRevisions.empty())
	{
		return { onStat, onOutput };
	}

	std::string argsString;
	for (const std::string& stringArg : fileRevisions)
	{
		argsString += " " + stringArg;
	}

	std::vector<char*> argsCharArray;
	argsCharArray.reserve(fileRevisions.size());
	for (const std::string& arg : fileRevisions)
	{
		argsCharArray.push_back((char*)arg.c_str());
	}

	PrintResult clientUser(onStat, onOutput);

	m_ClientAPI.SetArgv(argsCharArray.size(), argsCharArray.data());
	m_ClientAPI.Run("print", &clientUser);

	int retries = CommandRetries;
	while (m_ClientAPI.Dropped() || clientUser.GetError().IsError())
	{
		if (retries == 0)
		{
			break;
		}

		ERR("Connection dropped or command errored, retrying in 5 seconds.")
		std::this_thread::sleep_for(std::chrono::seconds(5));

		if (Reinitialize())
		{
			SUCCESS("Reinitialized P4API")
		}
		else
		{
			ERR("Could not reinitialize P4API")
		}

		WARN("Retrying: p4 print" << argsString)

		clientUser = PrintResult(onStat, onOutput);

		m_ClientAPI.SetArgv(argsCharArray.size(), argsCharArray.data());
		m_ClientAPI.Run("print", &clientUser);

		retries--;
	}

	if (m_ClientAPI.Dropped() || clientUser.GetError().IsFatal())
	{
		ERR("Exiting due to receiving errors even after retrying " << CommandRetries << " times")
		Deinitialize();
		std::exit(1);
	}

	m_Usage++;
	if (m_Usage > CommandRefreshThreshold)
	{
		int refreshRetries = CommandRetries;
		while (refreshRetries > 0)
		{
			WARN("Trying to refresh the connection due to age (" << m_Usage << " > " << CommandRefreshThreshold << ").")
			if (Reinitialize())
			{
				SUCCESS("Connection was refreshed")
				break;
			}
			ERR("Could not refresh connection due to old age. Retrying in 5 seconds")
			std::this_thread::sleep_for(std::chrono::seconds(5));

			refreshRetries--;
		}

		if (refreshRetries == 0)
		{
			ERR("Could not refresh the connection after " << CommandRetries << " retries. Exiting.")
			std::exit(1);
		}
	}

	return clientUser;
}

UsersResult P4API::Users()
{
	return Run<UsersResult>("users", {
	                                     "-a" // Include service accounts
	                                 });
}

InfoResult P4API::Info()
{
	return Run<InfoResult>("info", {});
}
