/*
 * 20250215
 *
 * Program Acknowledgment:
 *
 * Portions of this program's code, specifically within the TemperatureReport.h and HardwareThermometer.h files,
 * were completed with the assistance of the Gemini large language model from Google. （2.0 Flash Thinking Experimental with apps）
 *
 * This acknowledgment serves to express gratitude for Gemini's valuable assistance in code design,
 * logical implementation, and problem-solving throughout the development process.
 */


#ifndef TEMPERATURE_REPORT_H
#define TEMPERATURE_REPORT_H

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <limits>
#include "HardwareThermometer.h"
#include <type_traits>


// --- TemperatureStats 结构体 (全局定义) ---
struct TemperatureStats {
    TemperatureStats(): min(std::numeric_limits<int32_t>::max()),
                        max(std::numeric_limits<int32_t>::min()),
                        avg(0), count(0) {}
    int32_t min;
    int32_t max;
    int32_t avg;
    std::size_t count;
    std::string firstName;
};


// --- 温度报告模板函数 ---
template <typename ThermometerType>
void reportTemperatureData(  // **声明和定义合并，只有一个代码块**
    ThermometerType* thermometerPtr,
    std::ostringstream& reportStd,
    std::ostringstream& reportDetail,
    const std::string& colorCode,
    bool singleLine,
    int posTemp,
    int posRam,
    const std::string& categoryName,
    const std::string& targetLabel,     //指定显示的温度
    bool showDetail = true              //鼠标悬停时显示详细温度
) {
    if (!thermometerPtr) return;

    thermometerPtr->readProc();

    using ReadingType = typename ThermometerType::Reading;

    std::map<std::string, TemperatureStats> statByCategory;

    int32_t maxAbsTemp = std::numeric_limits<int32_t>::min();

    for (const auto& reading : thermometerPtr->get_readings()) {
        std::string key = reading.label;

        using StatsMapIterator = std::map<std::string, TemperatureStats>::iterator;
        StatsMapIterator its = statByCategory.find(key);
        //reportStd << key;

        if (its == statByCategory.end()) {
            its = statByCategory.insert({key, TemperatureStats()}).first;
            its->second.firstName = reading.label;
        }
        if (reading.temp_mc < its->second.min) its->second.min = reading.temp_mc;
        if (reading.temp_mc > its->second.max) its->second.max = reading.temp_mc;
        if (reading.temp_mc > maxAbsTemp) maxAbsTemp = reading.temp_mc;
        auto prevTotal = its->second.avg * its->second.count;
        its->second.count++;
        its->second.avg = (prevTotal + reading.temp_mc) / its->second.count;
    }

    auto its = statByCategory.find(targetLabel);
    if (maxAbsTemp >= 0 && (!posRam || (posTemp < posRam)) | its != statByCategory.end()) {
        reportStd << "<span fgcolor='" << colorCode << "'>" << std::setw(3) << statByCategory[targetLabel].max / 1000 << "°C</span>" << (singleLine ? " " : "");
    }

    if (showDetail) {
        if (!statByCategory.empty()) reportDetail << " " << categoryName << ": \n";
        for (const auto& pair : statByCategory) {
            const auto& its = pair.second;
            if (its.count == 1) {
                reportDetail << "    " << its.firstName << ": " << its.max / 1000 << "ºC \n";
            } else {
                reportDetail << "    \u2206" << its.max / 1000
                             << "ºC  \u2207" << its.min / 1000
                             << "ºC  \u222B" << its.avg / 1000
                             << "ºC  (" << its.count << " " << pair.first << ") \n";
            }
        }
    }
}

#endif // TEMPERATURE_REPORT_H