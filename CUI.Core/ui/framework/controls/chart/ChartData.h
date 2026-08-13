#pragma once
#include "../../core/Value.h"
#include <string>
#include <vector>

namespace CUI {

struct ChartSeries {
    std::string name;
    std::vector<float> values;
    D2D1_COLOR_F color{ 0.0f, 0.0f, 0.0f, 0.0f };
    bool hasColor = false;
};

struct ChartTick {
    float value = 0.0f;
    std::string label;
};

D2D1_COLOR_F ChartPaletteColor(int index);
std::string FormatChartNumber(float value);
void ComputeNiceScale(float minValue, float maxValue, int maxTicks,
                      float& niceMin, float& niceMax, std::vector<ChartTick>& ticks);
int CategoryCount(const std::vector<std::string>& categories, const std::vector<ChartSeries>& series);

} // namespace CUI
