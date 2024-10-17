#include "p4_helpers.h"

// Decodes paths from Perforce that may be encoded.
// Perforce encodes the following characters:
// '@' -> "%40"
// '#' -> "%23"
// '*' -> "%2A"
// '%' -> "%25"
std::string decodePath(const std::string& input)
{
	std::string result;
	for (size_t i = 0; i < input.size(); i++)
	{
		if (input[i] == '%' && i + 2 < input.size())
		{
			std::string hexValue = input.substr(i, 3);

			if (hexValue == "%40")
			{
				result += '@';
			}
			else if (hexValue == "%23")
			{
				result += '#';
			}
			else if (hexValue == "%2A")
			{
				result += '*';
			}
			else if (hexValue == "%25")
			{
				result += '%';
			}
			else
			{
				// If it's none of the above we just keep it as is, as these are
				// the only characters Perforce would have replaced.
				result += hexValue;
			}

			i += 2;
		}
		else
		{
			result += input[i];
		}
	}

	return result;
}
