#ifndef LABELS_CONVERSION_H
#define LABELS_CONVERSION_H
#include "p4_api.h"

int updateTags(P4API* p4, const std::string& depotPath, git_repository* repo);
std::string sanitizeLabelName(std::string label);
std::string trimSuffix(const std::string& str, const std::string& suffix);
std::string trimPrefix(const std::string& str, const std::string& prefix);
std::string getChangelistFromCommit(const git_commit* commit);

#endif // LABELS_CONVERSION_H
