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

#ifndef HARDWARE_THERMOMETER_H
#define HARDWARE_THERMOMETER_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <system_error>

// --- Forward Declarations ---
namespace Device {
    struct cpu_amd;
    struct cpu_intel;
    struct gpu_amd;
    struct gpu_intel;
    struct gpu_nvidia;
    struct nvme_m;
    struct nvme_g;

    static constexpr const char asusec[] = "asusec";
    static constexpr const char k10temp[] = "k10temp";
    static constexpr const char amdgpu[] = "amdgpu";
    static constexpr const char nvme[] = "nvme";
}

// --- Helper function implementations directly in HardwareThermometer.h ---
void abortApp(const char* reason); // 添加 abortApp 函数声明  (注意: 只有声明，没有定义)
bool readFile(const char* inputFile, std::vector<char>& buffer, bool mustExist = true); // 保留 readFile 函数声明
template <typename T> bool fromString(const std::string& from, T& to);

#define VA_STR(x) dynamic_cast<std::ostringstream const&>(std::ostringstream().flush() << x).str()


template <
    typename DeviceIdentifier,
    const char* DeviceName,
    int HwmonNumber,             // 已知的 hwmon 编号
    int LabelNumber,
    const char* LabelPattern = nullptr
>
class HardwareThermometer {
public:
    struct Reading {
        std::string label;
        int32_t temp_mc; // temperature in millidegree Celsius

        Reading(std::string l = "", int32_t t = 0) : label(l), temp_mc(t) {}
    };

protected:
    std::vector<Reading> readings;
    const char* deviceName;
    int hwmonNumber;             // 使用已知的 hwmon 编号
    int labelNumber;
    const char* labelPattern;


public:
    HardwareThermometer() : deviceName(DeviceName),
                            hwmonNumber(HwmonNumber), // 直接使用传入的 hwmonNumber
                            labelNumber(LabelNumber),
                            labelPattern(LabelPattern) {}
    virtual ~HardwareThermometer() {}

    const std::vector<Reading>& get_readings() const { return readings; }

    void readProc() {
        readings.clear();
        std::string coretempBaseDir;

        // 1. 直接构建 hwmon 目录路径，不再需要循环扫描
        coretempBaseDir = VA_STR("/sys/class/hwmon/hwmon" << hwmonNumber);
        std::vector<char> buffer;
        if (!readFile(VA_STR(coretempBaseDir << "/name").c_str(), buffer, false))
        {
            coretempBaseDir.append("/device"); // 尝试 /device 子目录
            if (!readFile(VA_STR(coretempBaseDir << "/name").c_str(), buffer, false)) {
                // 如果 name 文件仍然读取失败，则直接返回，不再继续读取温度
                return;
            }
        }
        std::istringstream sname(&buffer[0]);
        std::string name;
        if (!((sname >> name) && (name == deviceName))) {
             // 如果设备名称不匹配，也直接返回
             return;
        }


        // 2. 读取温度数据 (保持不变)
        if (!coretempBaseDir.empty()) {
            for (int ic = 0; ic < LabelNumber; ic++) {
                std::vector<char> buffer;
                std::ostringstream labelFileNameStream, inputFileNameStream;

                inputFileNameStream << coretempBaseDir << "/temp" << ic << "_input";
                labelFileNameStream << coretempBaseDir << "/temp" << ic << "_label";


                if (!readFile(labelFileNameStream.str().c_str(), buffer, false)) {
                    continue;
                }
                std::istringstream sname(&buffer[0]);
                std::string label;
                if (!std::getline(sname, label) || label.empty()) break;

                std::vector<char> tempBuffer;
                if (!readFile(inputFileNameStream.str().c_str(), tempBuffer, false)) break;

                std::istringstream tempStream(&tempBuffer[0]);
                int32_t tempMilliCelsius;
                if (!(tempStream >> tempMilliCelsius)) break;


                std::string lowerLabel = label;
                std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::tolower);

                if (labelPattern == nullptr || lowerLabel.find(labelPattern) != std::string::npos)
                {
                    readings.emplace_back(label, tempMilliCelsius);
                }
            }
        }
    }
};


// 使用模板类定义具体的温度传感器类 (需要根据已知的 hwmon 编号进行调整)
class Tcpu : public HardwareThermometer<Device::cpu_amd, Device::asusec, 5, 10> {};     // AMD CPU 对应的 hwmon 编号是 5
class Tccd : public HardwareThermometer<Device::cpu_amd, Device::k10temp, 4, 5> {};     // CCD 对应的 hwmon 编号是 4
class Tigpu : public HardwareThermometer<Device::gpu_amd, Device::amdgpu, 1, 5> {};     // iGPU 对应的 hwmon 编号是 1
class Tgpu : public HardwareThermometer<Device::gpu_amd, Device::amdgpu, 0, 5> {};      // GPU 对应的 hwmon 编号是 0
class Tnvme_m : public HardwareThermometer<Device::nvme_m, Device::nvme, 2, 5> {};        // NVME motherboard 对应的 hwmon 编号是 2
class Tnvme_g : public HardwareThermometer<Device::nvme_g, Device::nvme, 3, 5> {};        // NVME gen-z.2 pcie4 对应的 hwmon 编号是 3


#endif // HARDWARE_THERMOMETER_H