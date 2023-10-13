/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include <thread>
#include <map>
#include <atomic>
#include <mutex>
#include <sstream>
#include <typeinfo>
#include <csignal>

#include "common.h"

#include "utils/timer.h"
#include "utils/arguments.h"

#include "thread_pool.h"
#include "p4_api.h"
#include "git_api.h"
#include "branch_set.h"

#include "p4/p4libs.h"
#include "minitrace.h"

#define P4_FUSION_VERSION "v1.13.0"

void SignalHandler(sig_atomic_t s);

int Main(int argc, char** argv)
{
	Timer programTimer;

	Arguments::GetSingleton()->RequiredParameter("--path", "P4 depot path to convert to a Git repo.  If used with '--branch', this is the base path for the branches.");
	Arguments::GetSingleton()->RequiredParameter("--src", "Relative path where the git repository should be created. This path should be empty before running p4-fusion for the first time in a directory.");
	Arguments::GetSingleton()->RequiredParameter("--port", "Specify which P4PORT to use.");
	Arguments::GetSingleton()->RequiredParameter("--user", "Specify which P4USER to use. Please ensure that the user is logged in.");
	Arguments::GetSingleton()->RequiredParameter("--client", "Name/path of the client workspace specification.");
	Arguments::GetSingleton()->OptionalParameter("--lookAhead", "1", "How many CLs in the future, at most, shall we keep downloaded by the time it is to commit them?");
	Arguments::GetSingleton()->OptionalParameterList("--branch", "A branch to migrate under the depot path.  May be specified more than once.  If at least one is given and the noMerge option is false, then the Git repository will include merges between branches in the history.  You may use the formatting 'depot/path:git-alias', separating the Perforce branch sub-path from the git alias name by a ':'; if the depot path contains a ':', then you must provide the git branch alias.");
	Arguments::GetSingleton()->OptionalParameter("--noMerge", "false", "Disable performing a Git merge when a Perforce branch integrates (or copies, etc) into another branch.");
	Arguments::GetSingleton()->OptionalParameter("--networkThreads", std::to_string(std::thread::hardware_concurrency()), "Specify the number of threads in the threadpool for running network calls. Defaults to the number of logical CPUs.");
	Arguments::GetSingleton()->OptionalParameter("--printBatch", "1", "Specify the p4 print batch size.");
	Arguments::GetSingleton()->OptionalParameter("--maxChanges", "-1", "Specify the max number of changelists which should be processed in a single run. -1 signifies unlimited range.");
	Arguments::GetSingleton()->OptionalParameter("--retries", "10", "Specify how many times a command should be retried before the process exits in a failure.");
	Arguments::GetSingleton()->OptionalParameter("--refresh", "100", "Specify how many times a connection should be reused before it is refreshed.");
	Arguments::GetSingleton()->OptionalParameter("--fsyncEnable", "false", "Enable fsync() while writing objects to disk to ensure they get written to permanent storage immediately instead of being cached. This is to mitigate data loss in events of hardware failure.");
	Arguments::GetSingleton()->OptionalParameter("--includeBinaries", "false", "Do not discard binary files while downloading changelists.");
	Arguments::GetSingleton()->OptionalParameter("--flushRate", "1000", "Rate at which profiling data is flushed on the disk.");
	Arguments::GetSingleton()->OptionalParameter("--noColor", "false", "Disable colored output.");

	PRINT("p4-fusion " P4_FUSION_VERSION);

	Arguments::GetSingleton()->Initialize(argc, argv);
	if (!Arguments::GetSingleton()->IsValid())
	{
		PRINT("Usage:" + Arguments::GetSingleton()->Help());
		return 1;
	}

	const bool noColor = Arguments::GetSingleton()->GetNoColor() != "false";
	if (noColor)
	{
		Log::DisableColoredOutput();
	}

	const bool noMerge = Arguments::GetSingleton()->GetNoMerge() != "false";

	const std::string depotPath = Arguments::GetSingleton()->GetDepotPath();
	const std::string srcPath = Arguments::GetSingleton()->GetSourcePath();
	const bool fsyncEnable = Arguments::GetSingleton()->GetFsyncEnable() != "false";
	const bool includeBinaries = Arguments::GetSingleton()->GetIncludeBinaries() != "false";
	const int maxChanges = std::atoi(Arguments::GetSingleton()->GetMaxChanges().c_str());
	const int flushRate = std::atoi(Arguments::GetSingleton()->GetFlushRate().c_str());
	const std::vector<std::string> branchNames = Arguments::GetSingleton()->GetBranches();

	PRINT("Running p4-fusion from: " << argv[0]);

	if (!P4API::InitializeLibraries())
	{
		return 1;
	}

	// Set the signal here because it gets reset after P4API library is initialized.
	std::signal(SIGINT, SignalHandler);
	std::signal(SIGTERM, SignalHandler);

	P4API::P4PORT = Arguments::GetSingleton()->GetPort();
	P4API::P4USER = Arguments::GetSingleton()->GetUsername();

	TestResult serviceConnectionResult = P4API().TestConnection(5);
	if (serviceConnectionResult.HasError())
	{
		ERR("Error occurred while connecting to " << P4API::P4PORT << ": " << serviceConnectionResult.PrintError());
		return 1;
	}

	SUCCESS("Perforce server is available");

	P4API::P4CLIENT = Arguments::GetSingleton()->GetClient();
	P4API::ClientSpec = P4API().Client().GetClientSpec();

	if (P4API::ClientSpec.mapping.empty())
	{
		WARN("Received a client spec with no mappings. Did you use the correct corresponding P4PORT for the " + P4API::ClientSpec.client + " client spec?");
	}

	PRINT("Updated client workspace view " << P4API::ClientSpec.client << " with " << P4API::ClientSpec.mapping.size() << " mappings");

	P4API p4;

	InfoResult p4infoRes = p4.Info();
	if (p4infoRes.HasError()) {
		ERR("Failed to fetch Perforce server timezone: " << p4infoRes.PrintError());
		return 1;
	}
	int timezoneMinutes = p4infoRes.GetServerTimezoneMinutes();
	SUCCESS("Perforce server timezone is " << timezoneMinutes << " minutes");

	if (!p4.IsDepotPathValid(depotPath))
	{
		ERR("Depot path should begin with \"//\" and end with \"/...\". Please pass in the proper depot path and try again.");
		return 1;
	}

	if (!p4.IsDepotPathUnderClientSpec(depotPath))
	{
		ERR("The depot path specified is not under the " << P4API::ClientSpec.client << " client spec. Consider changing the client spec so that it does. Exiting.");
		return 1;
	}

	int networkThreads = 1;
	std::string networkThreadsStr = Arguments::GetSingleton()->GetNetworkThreads();
	if (!networkThreadsStr.empty())
	{
		networkThreads = std::atoi(networkThreadsStr.c_str());
	}

	int printBatch = 1;
	std::string printBatchStr = Arguments::GetSingleton()->GetPrintBatch();
	if (!printBatchStr.empty())
	{
		printBatch = std::atoi(printBatchStr.c_str());
	}

	int lookAhead = 1;
	std::string lookAheadStr = Arguments::GetSingleton()->GetLookAhead();
	if (!lookAheadStr.empty())
	{
		lookAhead = std::atoi(lookAheadStr.c_str());
	}

	std::string retriesStr = Arguments::GetSingleton()->GetRetries();
	if (!retriesStr.empty())
	{
		P4API::CommandRetries = std::atoi(retriesStr.c_str());
	}

	std::string refreshStr = Arguments::GetSingleton()->GetRefresh();
	if (!refreshStr.empty())
	{
		P4API::CommandRefreshThreshold = std::atoi(refreshStr.c_str());
	}

	BranchSet branchSet(P4API::ClientSpec.mapping, depotPath, branchNames, includeBinaries);

	bool profiling = false;
#if MTR_ENABLED
	profiling = true;
#endif

	// Setup trace file generation
	const std::string tracePath = (srcPath + (srcPath.back() == '/' ? "" : "/") + "trace.json");
	mtr_init(tracePath.c_str());
	MTR_META_PROCESS_NAME("p4-fusion");
	MTR_META_THREAD_NAME("Main Thread");
	MTR_META_THREAD_SORT_INDEX(0);

	PRINT("Perforce Port: " << P4API::P4PORT);
	PRINT("Perforce User: " << P4API::P4USER);
	PRINT("Perforce Client: " << P4API::P4CLIENT);
	PRINT("Depot Path: " << depotPath);
	PRINT("Network Threads: " << networkThreads);
	PRINT("Print Batch: " << printBatch);
	PRINT("Look Ahead: " << lookAhead);
	PRINT("Max Retries: " << retriesStr);
	PRINT("Max Changes: " << maxChanges);
	PRINT("Refresh Threshold: " << refreshStr);
	PRINT("Fsync Enable: " << fsyncEnable);
	PRINT("Include Binaries: " << includeBinaries);
	PRINT("Profiling: " << profiling << " (" << tracePath << ")");
	PRINT("Profiling Flush Rate: " << flushRate);
	PRINT("No Colored Output: " << noColor);
	PRINT("Inspecting " << branchSet.Count() << " branches");

	GitAPI git(fsyncEnable);

	if (!git.InitializeRepository(srcPath))
	{
		ERR("Could not initialize Git repository. Exiting.");
		return 1;
	}

	std::string resumeFromCL;
	if (git.IsHEADExists())
	{
		if (!git.IsRepositoryClonedFrom(depotPath))
		{
			ERR("Git repository at " << srcPath << " was not initially cloned with depotPath = " << depotPath << ". Exiting.");
			return 1;
		}

		resumeFromCL = git.DetectLatestCL();
		WARN("Detected last CL committed as CL " << resumeFromCL);
	}

	// Prepare the repository.
	git.CreateIndex();

	// Load mapping data from usernames to emails.
	PRINT("Requesting userbase details from the Perforce server");
	UsersResult usersRes = p4.Users();
	if (usersRes.HasError()) {
		ERR("Failed to retrieve user details for mapping: " << usersRes.PrintError());
		return 1;
	}
	const std::unordered_map<UsersResult::UserID, UsersResult::UserData> users = std::move(usersRes.GetUserEmails());
	SUCCESS("Received " << users.size() << " userbase details from the Perforce server");

	// Request changelists.
	PRINT("Requesting changelists to convert from the Perforce server");
	ChangesResult changesRes = p4.Changes(depotPath, resumeFromCL, maxChanges);
	if (changesRes.HasError()) {
		ERR("Failed to list changes: " << changesRes.PrintError());
		return 1;
	}
	std::vector<ChangeList> changes = std::move(changesRes.GetChanges());
	// Return early if we have no work to do
	if (changes.empty())
	{
		SUCCESS("Repository is up to date. Exiting.");
		return 0;
	}
	size_t changelistCount = changes.size();
	SUCCESS("Found " << changelistCount << " uncloned CLs starting from CL " << changes.front().number << " to CL " << changes.back().number);

	PRINT("Creating " << networkThreads << " network threads");
	ThreadPool pool;
	pool.Initialize(networkThreads);
	SUCCESS("Created " << pool.GetThreadCount() << " threads in thread pool");

	auto t = std::thread([&pool]()
		{
			// See if the threadpool encountered any exceptions.
			try
			{
				pool.RaiseCaughtExceptions();
			}
			catch (const std::exception& e)
			{
				// This is unrecoverable
				ERR("Threadpool encountered an exception: " << e.what());
				pool.ShutDown();
				std::exit(1);
			}
		}
	);

	// Go in the chronological order
	size_t lastDownloadedCL = 0;
	int startupDownloadsCount = 0;
	for (size_t currentCL = 0; currentCL < changelistCount && currentCL < lookAhead; currentCL++)
	{
		ChangeList& cl = changes.at(currentCL);

		pool.AddJob([&cl, &branchSet, printBatch](P4API* p4)
		{
			cl.StartDownload(p4, branchSet, printBatch);
		});

		lastDownloadedCL = currentCL;
		startupDownloadsCount++;
	}

	SUCCESS("Queued first " << startupDownloadsCount << " CLs up until CL " << changes.at(lastDownloadedCL).number << " for downloading");

	// Commit procedure start
	Timer commitTimer;

	for (size_t i = 0; i < changelistCount; i++)
	{
		ChangeList& cl = changes.at(i);

		// Ensure the files are downloaded before committing them to the repository
		cl.WaitForDownload();

		std::string fullName = cl.user;
		std::string email = "deleted@user";
		if (users.find(cl.user) != users.end())
		{
			fullName = users.at(cl.user).fullName;
			email = users.at(cl.user).email;
		}

		for (auto& branchGroup : cl.changedFileGroups->branchedFileGroups)
		{
			if (!branchGroup.targetBranch.empty())
			{
				git.SetActiveBranch(branchGroup.targetBranch);
			}

			for (auto& file : branchGroup.files)
			{
				if (file.IsDeleted())
				{
					git.RemoveFileFromIndex(file.GetRelativePath());
				}
				else
				{
					git.AddFileToIndex(file.GetRelativePath(), file.GetContents(), file.IsExecutable());
				}

				// No use for keeping the contents in memory once it has been added
				file.Clear();
			}

			std::string mergeFrom = "";
			if (branchGroup.hasSource && !noMerge)
			{
				// Only perform merging if the branch group explicitly declares that the change
				// has a source, and if the user wants merging.
				mergeFrom = branchGroup.sourceBranch;
			}

			std::string commitSHA = git.Commit(depotPath,
			    cl.number,
			    fullName,
			    email,
			    timezoneMinutes,
			    cl.description,
			    cl.timestamp,
			    mergeFrom);

			// For scripting/testing purposes...
			PRINT("COMMIT:" << commitSHA << ":" << cl.number << ":" << branchGroup.targetBranch << ":");
			SUCCESS(
			    "CL " << cl.number << " --> Commit " << commitSHA
			          << " with " << branchGroup.files.size() << " files"
			          << (branchGroup.targetBranch.empty()
			                     ? ""
			                     : (" to branch " + branchGroup.targetBranch))
			          << (branchGroup.sourceBranch.empty()
			                     ? ""
			                     : (" from branch " + branchGroup.sourceBranch))
			          << ".");
		}
		SUCCESS(
		    "CL " << cl.number << " with "
		          << cl.changedFileGroups->totalFileCount << " files (" << i + 1 << "/" << changelistCount
		          << "|" << lastDownloadedCL - (long long)i
		          << "). Elapsed " << commitTimer.GetTimeS() / 60.0f << " mins. "
		          << ((commitTimer.GetTimeS() / 60.0f) / (float)(i + 1)) * (changelistCount - i - 1) << " mins left.");
		// Clear out finished changelist.
		cl.Clear();

		// Start downloading the CL chronologically after the last CL that was previously downloaded, if there's still some left
		// TODO: We should start loading more data as soon as some download finished, IMO.
		if (lastDownloadedCL + 1 < changelistCount)
		{
			lastDownloadedCL++;
			ChangeList& downloadCL = changes.at(lastDownloadedCL);
			pool.AddJob([&downloadCL, &branchSet, printBatch](P4API* p4)
			{
				downloadCL.StartDownload(p4, branchSet, printBatch);
			});
		}

		// Occasionally flush the profiling data
		if ((i % flushRate) == 0)
		{
			mtr_flush();
		}
	}

	git.CloseIndex();

	SUCCESS("Completed conversion of " << changes.size() << " CLs in " << programTimer.GetTimeS() / 60.0f << " minutes, taking " << commitTimer.GetTimeS() / 60.0f << " to commit CLs");

	pool.ShutDown();

	// Wait for exception handler to finish.
	t.join();

	if (!P4API::ShutdownLibraries())
	{
		return 1;
	}

	// Finalize tracing.
	mtr_flush();
	mtr_shutdown();

	return 0;
}

void SignalHandler(sig_atomic_t s)
{
	static bool called = false;
	if (called)
	{
		WARN("Already received an interrupt signal, waiting for threads to shutdown.");
		return;
	}
	called = true;

	ERR("Signal Received: " << strsignal(s));

	// TODO: Propagate cancel signal to main function.
	// pool.ShutDown();

	std::exit(s);
}

int main(int argc, char** argv)
{
	int exitCode = 0;

	try
	{
		exitCode = Main(argc, argv);
	}
	catch (const std::exception& e)
	{
		ERR("Exception occurred: " << typeid(e).name() << ": " << e.what());
		return 1;
	}

	return exitCode;
}
