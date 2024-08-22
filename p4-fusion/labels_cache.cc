#include "labels_cache.h"

void writeString(std::ofstream& outFile, const std::string& str)
{
	size_t length = str.size();
	outFile.write(reinterpret_cast<const char*>(&length), sizeof(length));
	outFile.write(str.data(), length);
}

void writeVectorOfStrings(std::ofstream& outFile, const std::vector<std::string>& vec)
{
	size_t vecSize = vec.size();
	outFile.write(reinterpret_cast<const char*>(&vecSize), sizeof(vecSize));
	for (const auto& str : vec)
	{
		writeString(outFile, str);
	}
}

void writeStructToDisk(std::ofstream& outFile, const LabelResult& labels)
{
	writeString(outFile, labels.label);
	writeString(outFile, labels.revision);
	writeString(outFile, labels.description);
	writeString(outFile, labels.update);
	writeVectorOfStrings(outFile, labels.views);
}

void writeLabelMapToDisk(const std::string& filename, const LabelNameToDetails& labelMap, const std::string& cacheFile)
{
	std::ofstream outFile(filename, std::ios::binary);
	if (!outFile)
	{
		ERR("Error opening " << cacheFile << " file for writing, could not cache labels!");
		return;
	}

	// Write the size of the map
	size_t size = labelMap.size();
	outFile.write(reinterpret_cast<const char*>(&size), sizeof(size));

	// Write each key-value pair
	for (const auto& pair : labelMap)
	{
		writeStructToDisk(outFile, pair.second);
	}

	outFile.close();
}

std::string readString(std::ifstream& inFile)
{
	size_t length;
	inFile.read(reinterpret_cast<char*>(&length), sizeof(length));
	std::string str(length, '\0');
	inFile.read(&str[0], length);
	return str;
}

std::vector<std::string> readVectorOfStrings(std::ifstream& inFile)
{
	size_t vecSize;
	inFile.read(reinterpret_cast<char*>(&vecSize), sizeof(vecSize));
	std::vector<std::string> vec(vecSize);
	for (size_t i = 0; i < vecSize; ++i)
	{
		vec[i] = readString(inFile);
	}
	return vec;
}

LabelResult readStructFromDisk(std::ifstream& inFile)
{
	LabelResult label;
	label.label = readString(inFile);
	label.revision = readString(inFile);
	label.description = readString(inFile);
	label.update = readString(inFile);
	label.views = readVectorOfStrings(inFile);
	return label;
}

LabelNameToDetails readLabelMapFromDisk(const std::string& filename)
{
	LabelNameToDetails labelMap;
	std::ifstream inFile(filename, std::ios::binary);
	if (!inFile)
	{
		ERR("No label cache found at " << filename)
		return labelMap;
	}

	// Read the size of the map
	size_t size;
	inFile.read(reinterpret_cast<char*>(&size), sizeof(size));

	// Read each key-value pair and insert into the map
	for (size_t i = 0; i < size; ++i)
	{
		LabelResult value = readStructFromDisk(inFile);
		labelMap.insert({ value.label, value });
	}

	inFile.close();
	return labelMap;
}

// Compares the last updated date in the labels list to the updated dates
// in the cache, and returns all labels of which the last updated date is
// different.
compareResponse compareLabelsToCache(const std::list<LabelsResult::LabelData>& labels, LabelNameToDetails& labelMap)
{
	LabelNameToDetails newLabelMap;
	std::list<LabelsResult::LabelData> labelsToFetch;
	for (const auto& label : labels)
	{
		if (labelMap.contains(label.label))
		{
			LabelResult cachedLabel
			    = labelMap.at(label.label);
			if (cachedLabel.update != label.update)
			{
				labelsToFetch.push_back(label);
			}
			else
			{
				newLabelMap.insert({ label.label, cachedLabel });
			}
		}
		else
		{
			labelsToFetch.push_back(label);
		}
	}

	return compareResponse {
		.labelsToFetch = labelsToFetch,
		.resultingLabels = newLabelMap
	};
}
