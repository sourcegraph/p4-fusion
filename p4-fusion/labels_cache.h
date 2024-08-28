#include <fstream>
#include <string>
#include <vector>

#include "labels_conversion.h"

struct CompareResponse
{
	std::list<LabelsResult::LabelData> labelsToFetch;
	LabelNameToDetails resultingLabels; // labelMap with the labels that no longer exist removed
};

void write_label_map_to_disk(const std::string& filename, const LabelNameToDetails& labelMap, const std::string& cacheFile);
LabelNameToDetails read_label_map_from_disk(const std::string& filename);
CompareResponse compare_labels_to_cache(const std::list<LabelsResult::LabelData>& labels, LabelNameToDetails& labelMap);
