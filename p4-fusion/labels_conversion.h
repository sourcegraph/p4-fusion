#ifndef LABELS_CONVERSION_H
#define LABELS_CONVERSION_H
#include "commands/labels_result.h"
#include "p4_api.h"

// A map from a label name to the details of the label
using LabelNameToDetails = std::unordered_map<std::string, LabelResult>;

std::string convertLabelToTag(std::string label);
std::string trimPrefix(const std::string& str, const std::string& prefix);
std::string getChangelistFromCommit(const git_commit* commit);
LabelMap labelDetailsToMap(std::string depotPath, LabelNameToDetails labels);
LabelNameToDetails getLabelsDetails(P4API* p4, std::list<LabelsResult::LabelData> labels);

#endif // LABELS_CONVERSION_H
