/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */

#include <csignal>
#include <cerrno>
#include <sstream>

#include "thread_pool.h"
#include "signalhandler.h"
#include "thread.h"
#include "log.h"

int signalThread(std::thread& t);

SignalHandler::SignalHandler(ThreadPool& pool)
{
	// Block signals from being handled by the main thread, and all future threads.
	//
	// - SIGINT, SIGTERM, SIGHUP: thread pool will be shutdown and std::exit will be called
	// - SIGUSR1: only sent by ~SignalHandler() to tell the signal handler thread to exit
	sigset_t blockedSignals;
	sigemptyset(&blockedSignals);
	for (auto sig : { SIGINT, SIGTERM, SIGHUP, SIGUSR1 })
	{
		sigaddset(&blockedSignals, sig);
	}

	int rc = pthread_sigmask(SIG_BLOCK, &blockedSignals, nullptr);
	if (rc != 0)
	{
		std::ostringstream oss;
		oss << "(signal handler) failed to block signals: (" << errno << ") " << strerror(errno);
		throw std::runtime_error(oss.str());
	}

	sigset_t signalsToWaitOn = blockedSignals;
	// Spawn a thread to handle signals.
	// The thread will block and wait for signals to arrive and then shutdown the thread pool, unless it receives SIGUSR1,
	// in which case it will just exit (since main() is handling the shutdown).
	//
	// Using a separate thread for purely signal handling allows us to use non-reentrant functions
	// (such as std::cout, condition variables, etc.) in the signal handler.
	signalHandlingThread = ThreadRAII(std::thread(
	    [signalsToWaitOn, &pool]()
	    {
		    // Wait for signals to arrive.
		    int sig;
		    int rc = sigwait(&signalsToWaitOn, &sig);
		    if (rc != 0)
		    {
			    ERR("(signal handler) failed to wait for signals: (" << errno << ") " << strerror(errno))
			    pool.ShutDown();
			    std::exit(errno);
		    }

		    // Did main() tell us to shutdown?
		    if (sig == SIGUSR1)
		    {
			    // Yes, so no need to print anything - just exit.
			    return;
		    }

		    // Otherwise, we received a signal from the OS - print a message and shutdown.
		    if (!sigismember(&signalsToWaitOn, sig))
		    {
			    ERR("(signal handler): WARNING: received signal (" << sig << ") \"" << strsignal(sig) << "\" that is not blocked, this should not happen and indicates a logic error in the signal handler.")
		    }

		    ERR("(signal handler) received signal (" << sig << ") \"" << strsignal(sig) << "\", shutting down")
		    pool.ShutDown();
		    std::exit(sig);
	    }));
}

SignalHandler::~SignalHandler()
{
	// Send a signal to the handling thread to make it shut down.
	// Once this function returns, the destructor of the nested
	// ThreadRAII is called and will join on it to make sure it
	// exited properly, so we need to be sure that it will quit
	// really soon :tm:.
	int errcode = signalThread(signalHandlingThread.get());
	if (errcode != 0)
	{
		ERR("(signal handler) failed to shut down signal handling thread: (" << errcode << ") " << strerror(errcode))
	}
}

int signalThread(std::thread& t)
{
	auto rc = pthread_kill(t.native_handle(), SIGUSR1);
	if (rc != 0)
	{
		return errno;
	}

	SUCCESS("Signal handler shut down successfully")
	return 0;
}
