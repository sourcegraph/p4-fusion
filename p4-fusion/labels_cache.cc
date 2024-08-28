#include "labels_cache.h"

const int LABEL_CACHE_VERSION = 1;

void write_int(std::ofstream& outFile, const int number)
{
	outFile.write(reinterpret_cast<const char*>(&number), sizeof(number));
}

void write_string(std::ofstream& outFile, const std::string& str)
{
	size_t length = str.size();
	outFile.write(reinterpret_cast<const char*>(&length), sizeof(length));
	outFile.write(str.data(), length);
}

void write_vector_of_strings(std::ofstream& outFile, const std::vector<std::string>& vec)
{
	size_t vecSize = vec.size();
	outFile.write(reinterpret_cast<const char*>(&vecSize), sizeof(vecSize));
	for (const auto& str : vec)
	{
		write_string(outFile, str);
	}
}

void write_struct_to_disk(std::ofstream& outFile, const LabelResult& labels)
{
	write_int(outFile, LABEL_CACHE_VERSION); // Write a version number first
	write_string(outFile, labels.label);
	write_string(outFile, labels.revision);
	write_string(outFile, labels.description);
	write_string(outFile, labels.update);
	write_vector_of_strings(outFile, labels.views);
}

void write_label_map_to_disk(const std::string& filename, const LabelNameToDetails& labelMap, const std::string& cacheFile)
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
		write_struct_to_disk(outFile, pair.second);
	}

	outFile.close();
}

int read_int(std::ifstream& inFile)
{
	int number;
	inFile.read(reinterpret_cast<char*>(&number), sizeof(number));
	return number;
}

std::string read_string(std::ifstream& inFile)
{
	size_t length;
	inFile.read(reinterpret_cast<char*>(&length), sizeof(length));
	std::string str(length, '\0');
	inFile.read(&str[0], length);
	return str;
}

std::vector<std::string> read_vector_of_strings(std::ifstream& inFile)
{
	size_t vecSize;
	inFile.read(reinterpret_cast<char*>(&vecSize), sizeof(vecSize));
	std::vector<std::string> vec(vecSize);
	for (size_t i = 0; i < vecSize; ++i)
	{
		vec[i] = read_string(inFile);
	}
	return vec;
}

LabelResult read_struct_from_disk(std::ifstream& inFile)
{
	LabelResult label;
	int version = read_int(inFile);
	if (version != LABEL_CACHE_VERSION)
	{
		ERR("Incorrect label version!")
		return label;
	}
	label.label = read_string(inFile);
	label.revision = read_string(inFile);
	label.description = read_string(inFile);
	label.update = read_string(inFile);
	label.views = read_vector_of_strings(inFile);
	return label;
}

LabelNameToDetails read_label_map_from_disk(const std::string& filename)
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
		LabelResult value = read_struct_from_disk(inFile);
		labelMap.insert({ value.label, value });
	}

	inFile.close();
	return labelMap;
}

// Compares the last updated date in the labels list to the updated dates
// in the cache, and returns all labels of which the last updated date is
// different.
CompareResponse compare_labels_to_cache(const std::list<LabelsResult::LabelData>& labels, LabelNameToDetails& cachedLabelMap)
{
	LabelNameToDetails newLabelMap;
	std::list<LabelsResult::LabelData> labelsToFetch;
	for (const auto& label : labels)
	{
		if (cachedLabelMap.contains(label.label))
		{
			LabelResult cachedLabel
			    = cachedLabelMap.at(label.label);
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

	return CompareResponse {
		.labelsToFetch = labelsToFetch,
		.resultingLabels = newLabelMap
	};
}
