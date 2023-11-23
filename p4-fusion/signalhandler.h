/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include "thread.h"
#include "thread_pool.h"

/*
 * SignalHandler wraps the ThreadRAII for the signal handling thread to ensure
 * it is properly signaled to quit on exit.
 */
class SignalHandler
{
public:
	SignalHandler() = delete;
	explicit SignalHandler(ThreadPool& pool);
	~SignalHandler();

private:
	ThreadRAII signalHandlingThread;
};
