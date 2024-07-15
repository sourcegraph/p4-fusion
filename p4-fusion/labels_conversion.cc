#include <string>

#include "p4_api.h"
#include "git2/commit.h"
#include "git2/types.h"

std::string sanitizeLabelName(std::string label)
{
	// Remove leading slashes
	while (!label.empty() && label.front() == '/')
	{
		label.erase(label.begin());
	}

	// Remove trailing slashes and dots
	while (!label.empty() && (label.back() == '/' || label.back() == '.'))
	{
		label.pop_back();
	}

	// Replace specific characters with underscores
	const std::string charsToReplace = " ~^:?*[@{";
	for (char& ch : label)
	{
		if (charsToReplace.find(ch) != std::string::npos)
		{
			ch = '_';
		}
	}

	// Replace "//" with "/"
	const std::string doubleSlash = "//";
	const std::string singleSlash = "/";
	std::string::size_type pos = 0;
	while ((pos = label.find(doubleSlash, pos)) != std::string::npos)
	{
		label.replace(pos, doubleSlash.length(), singleSlash);
	}

	// If the label is exactly "@", return an empty string
	if (label == "@")
	{
		return "";
	}

	return label;
}

// Function to trim the specified suffix from the string
std::string trimSuffix(const std::string& str, const std::string& suffix)
{
	if (str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0)
	{
		return str.substr(0, str.size() - suffix.size());
	}
	return str;
}

// Function to trim the specified prefix from the string
std::string trimPrefix(const std::string& str, const std::string& prefix)
{
	if (str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0)
	{
		return str.substr(prefix.size());
	}
	return str;
}

std::string getChangelistFromCommit(const git_commit* commit)
{
	// Look for the specific change message generated from the Commit method.
	// Note that extra branching information can be added after it.
	// ": change = " is 11 characters long.
	std::string message = git_commit_message(commit);
	const size_t pos = message.find(": change = ");
	if (pos == std::string::npos)
	{
		throw std::invalid_argument("failed to parse commit message, well known section : change =  not found");
	}
	// The changelist number starts after 11 characters, the length of ": change = ".
	const size_t clStart = pos + 11;
	const size_t clEnd = message.find(']', clStart);
	if (clEnd == std::string::npos)
	{
		throw std::invalid_argument("failed to parse commit message, well closing ] not found");
	}

	std::string cl(message, clStart, clEnd - clStart);

	return cl;
}