/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "print_result.h"

#include <utility>

void PrintResult::PrintResultIterator::OnStat()
{
	// Noop.
}

void PrintResult::PrintResultIterator::OnOutput(const char*, int)
{
	// Noop.
}

PrintResult::PrintResult(std::shared_ptr<PrintResult::PrintResultIterator> _it)
    : it(_it)
{
}

void PrintResult::OutputStat(StrDict* varList)
{
	it->OnStat();
}

void PrintResult::OutputText(const char* data, int length)
{
	it->OnOutput(data, length);
}

void PrintResult::OutputBinary(const char* data, int length)
{
	it->OnOutput(data, length);
}
