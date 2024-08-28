#ifndef LABELS_CONVERSION_H
#define LABELS_CONVERSION_H
#include "commands/labels_result.h"
#include "p4_api.h"

// A map from a label name to the details of the label
using LabelNameToDetails = std::unordered_map<std::string, LabelResult>;

std::string convert_label_to_tag(std::string label);
std::string trim_prefix(const std::string& str, const std::string& prefix);
std::string get_changelist_from_commit(const git_commit* commit);
LabelMap label_details_to_map(std::string depotPath, LabelNameToDetails labels);
LabelNameToDetails get_labels_details(P4API* p4, std::list<LabelsResult::LabelData> labels);

#endif // LABELS_CONVERSION_H
