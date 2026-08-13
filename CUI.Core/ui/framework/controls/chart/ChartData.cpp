#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ChartData.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace CUI {

D2D1_COLOR_F ChartPaletteColor(int index) {
    static const unsigned int kPalette[] = {
        0x0078D4u, 0x13A10Eu, 0xD83B01u, 0x8764B8u,
        0x00B7C3u, 0xE74856u, 0xFFB900u, 0xC239B3u
    };
    const unsigned int rgb = kPalette[index < 0 ? 0 : (index % 8)];
    return D2D1::ColorF(
        static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f,
        static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f,
        static_cast<float>(rgb & 0xFFu) / 255.0f,
        1.0f);
}

std::string FormatChartNumber(float value) {
    if (!std::isfinite(value)) {
        return "–";
    }
    const float absV = std::abs(value);
    char buf[32];
    if (absV >= 1000000.0f) {
        sprintf_s(buf, "%.1fM", value / 1000000.0f);
    } else if (absV >= 10000.0f) {
        sprintf_s(buf, "%.1fK", value / 1000.0f);
    } else if (std::abs(value - std::round(value)) < 0.05f) {
        sprintf_s(buf, "%.0f", value);
    } else {
        sprintf_s(buf, "%.1f", value);
    }
    return buf;
}

namespace {
float NiceNum(float range, bool round) {
    if (!(range > 0.0f) || !std::isfinite(range)) {
        return 1.0f;
    }
    const float exp = std::floor(std::log10(range));
    const float f = range / std::pow(10.0f, exp);
    float nf = 10.0f;
    if (round) {
        if (f < 1.5f) nf = 1.0f;
        else if (f < 3.0f) nf = 2.0f;
        else if (f < 7.0f) nf = 5.0f;
    } else {
        if (f <= 1.0f) nf = 1.0f;
        else if (f <= 2.0f) nf = 2.0f;
        else if (f <= 5.0f) nf = 5.0f;
    }
    return nf * std::pow(10.0f, exp);
}
} // namespace

void ComputeNiceScale(float minValue, float maxValue, int maxTicks,
                      float& niceMin, float& niceMax, std::vector<ChartTick>& ticks) {
    ticks.clear();
    maxTicks = std::clamp(maxTicks, 2, 10);
    if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
        minValue = 0.0f;
        maxValue = 1.0f;
    }
    if (std::abs(maxValue - minValue) < 1.0e-4f) {
        if (std::abs(maxValue) < 1.0e-4f) {
            minValue = 0.0f;
            maxValue = 1.0f;
        } else {
            minValue -= std::abs(minValue) * 0.2f;
            maxValue += std::abs(maxValue) * 0.2f;
        }
    }
    if (maxValue < minValue) {
        std::swap(minValue, maxValue);
    }

    const float range = NiceNum(maxValue - minValue, false);
    const float tick = NiceNum(range / static_cast<float>(maxTicks - 1), true);
    niceMin = std::floor(minValue / tick) * tick;
    niceMax = std::ceil(maxValue / tick) * tick;
    if (std::abs(niceMax - niceMin) < tick * 0.5f) {
        niceMax = niceMin + tick;
    }

    for (float v = niceMin; v <= niceMax + tick * 0.5f; v += tick) {
        ChartTick t;
        t.value = v;
        t.label = FormatChartNumber(v);
        ticks.push_back(std::move(t));
        if (static_cast<int>(ticks.size()) > 12) {
            break;
        }
    }
}

int CategoryCount(const std::vector<std::string>& categories, const std::vector<ChartSeries>& series) {
    int count = static_cast<int>(categories.size());
    for (const auto& s : series) {
        count = (std::max)(count, static_cast<int>(s.values.size()));
    }
    return count;
}

} // namespace CUI
