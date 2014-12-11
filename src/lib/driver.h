#pragma once

#include <string>


namespace mhc {

	namespace driver {

		bool generateOutput(const std::string& input, const std::string& outputBitCodeName);

		bool optimizeAndLink(const std::string& bitCodeFilename, const std::string& exeName = "");

	}

}

