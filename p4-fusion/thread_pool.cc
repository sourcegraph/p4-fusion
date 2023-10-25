/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "thread_pool.h"
#include "common.h"
#include "p4_api.h"
#include "minitrace.h"
#include "git_api.h"

void ThreadPool::AddJob(const Job& function)
{
	// Check if we want to accept more work.
	if (m_HasShutDownBeenCalled)
	{
		return;
	}

	{
		std::unique_lock<std::mutex> lock(m_JobsMutex);
		m_Jobs.push_back(function);
	}
	// Inform the next available job handler that there's new work.
	m_CV.notify_one();
}

void ThreadPool::RaiseCaughtExceptions()
{
	while (!m_HasShutDownBeenCalled)
	{
		std::unique_lock<std::mutex> lock(m_ThreadExceptionsMutex);
		m_ThreadExceptionCV.wait(lock);

		while (!m_ThreadExceptions.empty())
		{
			std::exception_ptr ex = m_ThreadExceptions.front();
			m_ThreadExceptions.pop_front();
			if (ex)
			{
				std::rethrow_exception(ex);
			}
		}
	}
}

void ThreadPool::ShutDown()
{
	if (m_HasShutDownBeenCalled)
	{
		return;
	}
	m_HasShutDownBeenCalled = true;

	{
		std::lock_guard<std::mutex> lock(m_JobsMutex);
		m_ShouldStop = true;
	}
	m_CV.notify_all();
	m_ThreadExceptionCV.notify_all();

	// Wait for all worker threads to finish.
	for (auto& thread : m_Threads)
	{
		thread.join();
	}

	m_Threads.clear();

	{
		std::lock_guard<std::mutex> lock(m_ThreadExceptionsMutex);
		m_ThreadExceptions.clear();
	}

	SUCCESS("Thread pool shut down successfully")
}

ThreadPool::ThreadPool(const int size, const std::string& repoPath, const bool fsyncEnable, const int tz)
    : m_HasShutDownBeenCalled(false)
    , m_ShouldStop(false)
{
	// Initialize size threads.
	for (int i = 0; i < size; i++)
	{
		m_Threads.emplace_back([this, i, &repoPath, &fsyncEnable, &tz]()
		    {
				// Add some human-readable info to the tracing.
				MTR_META_THREAD_NAME(("Worker #" + std::to_string(i)).c_str());

			    // Initialize p4 API.
			    P4API p4;
			    GitAPI git(repoPath, fsyncEnable, tz);

			    git.OpenRepository();

				// Job queue, we keep looking for new jobs until the shutdown
				// event.
				while (true)
				{
					Job job;
					{
						std::unique_lock<std::mutex> lock(m_JobsMutex);

						m_CV.wait(lock, [this]()
							{ return !m_Jobs.empty() || m_ShouldStop; });

						if (m_ShouldStop)
						{
							break;
						}

						job = m_Jobs.front();
						m_Jobs.pop_front();
					}

					try
					{
						job(p4, git);
					}
					catch (const std::exception& e)
					{
						std::unique_lock<std::mutex> lock(m_ThreadExceptionsMutex);
						m_ThreadExceptions.push_back(std::current_exception());
						m_ThreadExceptionCV.notify_all();
					}
				} });
	}
}

ThreadPool::~ThreadPool()
{
	ShutDown();
}
