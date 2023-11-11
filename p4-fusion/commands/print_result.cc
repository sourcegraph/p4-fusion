/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "print_result.h"

#include <utility>

PrintResult::PrintResult(std::function<void()> stat, std::function<void(const char*, int)> out): onStat(std::move(stat)),onOutput(std::move(out)) {}

void PrintResult::OutputStat(StrDict* varList)
{
	onStat();
}

void PrintResult::OutputText(const char* data, int length)
{
	onOutput(data, length);
}

void PrintResult::OutputBinary(const char* data, int length)
{
	OutputText(data, length);
}
