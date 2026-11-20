/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include "common.h"

class Result : public ClientUser
{
	Error m_Error;

public:
	Result() = default;
	// ClientUser owns a heap-allocated mutex (setterGuard) and frees it in its
	// destructor, so its implicit copy makes two objects free the same mutex.
	// Copies get a fresh ClientUser instead. Error's implicit copy constructor
	// is shallow too, but its assignment operator copies deeply.
	Result(const Result& other)
	    : ClientUser()
	{
		m_Error = other.m_Error;
	}
	Result& operator=(const Result& other)
	{
		m_Error = other.m_Error;
		return *this;
	}

	void HandleError(Error* e) override;

	[[nodiscard]] const Error& GetError() const { return m_Error; }
	/*
	 * HasError returns true when the result contains an error.
	 */
	[[nodiscard]] bool HasError() const
	{
		return GetError().IsError();
	}
	/*
	 * PrintError formats the contained error, if any, as a std::string.
	 */
	[[nodiscard]] std::string PrintError() const
	{
		if (!HasError())
		{
			return "";
		}
		StrBuf str;
		m_Error.Fmt(&str);
		return { str.Text() };
	}
};
