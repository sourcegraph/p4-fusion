#ifndef LABELS_CONVERSION_H
#define LABELS_CONVERSION_H
#include "p4_api.h"

int updateTags(P4API* p4, const std::string& depotPath, git_repository* repo);
std::string convertLabelToTag(std::string label);
std::string trimSuffix(const std::string& str, const std::string& suffix);
std::string trimPrefix(const std::string& str, const std::string& prefix);
std::string getChangelistFromCommit(const git_commit* commit);
LabelMap getLabelsDetails(P4API* p4, std::string depotPath, std::list<std::string> labels);

#endif // LABELS_CONVERSION_H
