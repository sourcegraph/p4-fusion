/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include <vector>
#include <functional>
#include "common.h"
#include "result.h"

class PrintResult : public Result
{
public:
	class PrintResultIterator
	{
	public:
		virtual void OnStat();
		virtual void OnOutput(const char*, int);
	};

private:
	std::shared_ptr<PrintResultIterator> it;

public:
	PrintResult() = delete;
	PrintResult(std::shared_ptr<PrintResultIterator> it);
	void OutputStat(StrDict* varList) override;
	void OutputText(const char* data, int length) override;
	void OutputBinary(const char* data, int length) override;
};
