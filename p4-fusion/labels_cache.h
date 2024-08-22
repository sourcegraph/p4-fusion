#include <fstream>
#include <string>
#include <vector>

#include "labels_conversion.h"

struct compareResponse
{
	std::list<LabelsResult::LabelData> labelsToFetch;
	LabelNameToDetails resultingLabels; // labelMap with the labels that no longer exit removed
};

void writeLabelMapToDisk(const std::string& filename, const LabelNameToDetails& labelMap, const std::string& cacheFile);
LabelNameToDetails readLabelMapFromDisk(const std::string& filename);
compareResponse compareLabelsToCache(const std::list<LabelsResult::LabelData>& labels, LabelNameToDetails& labelMap);
