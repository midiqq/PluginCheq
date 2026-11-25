#pragma once

#include "AnalysisTypes.h"
#include <cstddef>
#include <vector>

namespace dsp {

class WindowFunctions {
public:
    static void generate(WindowType type, std::size_t size, std::vector<double>& outWindow);

    static double coherentGain(const std::vector<double>& window) noexcept;
    static double energyGain(const std::vector<double>& window) noexcept;
};

} // namespace dsp
