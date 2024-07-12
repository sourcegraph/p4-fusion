#include <string>

#include "p4_api.h"
#include "git2/commit.h"
#include "git2/refs.h"
#include "git2/repository.h"
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
	if (str.size() >= suffix.size() &&
		str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0)
	{
		return str.substr(0, str.size() - suffix.size());
	}
	return str;
}

// Function to trim the specified prefix from the string
std::string trimPrefix(const std::string& str, const std::string& prefix)
{
	if (str.size() >= prefix.size() &&
		str.compare(0, prefix.size(), prefix) == 0)
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

int updateTags(P4API* p4, const std::string& depotPath, git_repository* repo)
{
	PRINT("Updatings tags")

	// Load labels
	PRINT("Requesting labels from the Perforce server")
	LabelsResult labelsRes = p4->Labels();
	if (labelsRes.HasError())
	{
		ERR("Failed to retrieve labels for mapping: " << labelsRes.PrintError())
		return 1;
	}
	const std::list<std::string>& labels = labelsRes.GetLabels();
	SUCCESS("Received " << labels.size() << " labels from the Perforce server")

	std::unordered_map<std::string, std::unordered_map<std::string, LabelResult>*> revToLabel;

	for (auto& label : labelsRes.GetLabels())
	{
		LabelResult labelRes = p4->Label(label);
		if (labelRes.HasError())
		{
			ERR("Failed to retrieve label details: " << labelRes.PrintError());
			continue;
		}
		if (!labelRes.revision.starts_with("@"))
		{
			continue;
		}
		labelRes.revision.erase(labelRes.revision.begin());
		if (labelRes.views.empty())
		{
			if (!revToLabel.contains(labelRes.revision))
			{
				auto* newMap = new std::unordered_map<std::string, LabelResult>;
				revToLabel.insert({ labelRes.revision, newMap });
			}
			auto res = revToLabel.at(labelRes.revision);
			res->insert({ sanitizeLabelName(labelRes.label), labelRes });
		}
		else
		{
			for (auto& view : labelRes.views)
			{
				if (depotPath.starts_with(trimSuffix(view, "...")))
				{
					if (!revToLabel.contains(labelRes.revision))
					{
						auto* newMap = new std::unordered_map<std::string, LabelResult>;
						revToLabel.insert({ labelRes.revision, newMap });
					}
					auto res = revToLabel.at(labelRes.revision);
					res->insert({ sanitizeLabelName(labelRes.label), labelRes });
				}
			}
		}
	}

	git_reference_iterator* refIter;
	checkGit2Error(git_reference_iterator_glob_new(&refIter, repo, "refs/tags/*"));
	git_reference* ref;
	while (git_reference_next(&ref, refIter) >= 0)
	{
		std::string labelName = trimPrefix(git_reference_name(ref), "refs/tags/");

		git_commit* commit;
		checkGit2Error(git_commit_lookup(&commit, repo, git_reference_target(ref)));
		if (std::string cl = getChangelistFromCommit(commit); revToLabel.contains(cl) && revToLabel.at(cl)->contains(labelName))
		{
			revToLabel.at(cl)->erase(labelName);
			if (revToLabel.at(cl)->empty())
			{
				revToLabel.erase(cl);
			}
		}
		else
		{
			PRINT("Tag has moved or no longer exists, deleting: " << labelName)
			checkGit2Error(git_reference_delete(ref));
		}
	}

	PRINT("Creating new tags, if any...")

	git_reference* head;
	checkGit2Error(git_repository_head(&head, repo));

	git_commit* commit;
	checkGit2Error(git_commit_lookup(&commit, repo, git_reference_target(head)));

	while (true)
	{
		std::string clID = getChangelistFromCommit(commit);
		if (revToLabel.contains(clID))
		{
			for (auto& [_, v] : *revToLabel.at(clID))
			{
				SUCCESS("Creating tag " << sanitizeLabelName(v.label) << " for CL " << clID)
				git_reference* tmpref;
				checkGit2Error(git_reference_create(&tmpref, repo, ("refs/tags/" + sanitizeLabelName(v.label)).c_str(), git_commit_id(commit), false, v.description.c_str()));
			}
		}
		if (git_commit_parentcount(commit) == 0)
		{
			break;
		}
		checkGit2Error(git_commit_parent(&commit, commit, 0));
	}

	return 0;
}