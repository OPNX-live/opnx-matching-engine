//
// Created by Bob   on 2021/12/20.
//

#ifndef MATCHING_ENGINE_UTILS_H
#define MATCHING_ENGINE_UTILS_H

#include <chrono>
#include <ctime>
#include <atomic>
#include <regex>
#include <vector>
#include <string>

#include "json.hpp"


// The following global variables need to be defined in the main file
extern std::atomic<unsigned long long> g_ullSortId;
extern std::atomic<unsigned long long> g_ullMatchId;
extern std::atomic<unsigned long long> g_ullSequenceNumber;
extern unsigned long long g_ullNodeId;

namespace OPNX {
    class Utils {
    public:
        static inline unsigned long long getTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            return seconds;
        }
        static inline unsigned long long getMilliTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            return milliseconds;
        }
        static inline unsigned long long getMicroTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
            return milliseconds;
        }
        static inline unsigned long long getNanoTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            return milliseconds;
        }
        static inline void setNodeId(unsigned long long nodeId)
        {
            // The g_ullNodeId has only 4 bits, 14 bits were reserved as the g_ullSortId
            if (0 == g_ullNodeId) {
                std::string strNodeId = std::to_string(nodeId);
                std::string strSub = strNodeId.substr(0, 4);
                g_ullNodeId = std::stoi(strSub) * 100000000000000;
                g_ullSortId.fetch_add(g_ullNodeId, std::memory_order_relaxed);
                g_ullMatchId.fetch_add(g_ullNodeId, std::memory_order_relaxed);
            }
        }
        static inline unsigned long long getSortId()
        {
            // 4 bits were reserved as the nodeID
            return g_ullSortId.fetch_add(1, std::memory_order_relaxed);
        }
        static inline unsigned long long getMatchId()
        {
            // 4 bits were reserved as the nodeID
            return (g_ullMatchId.fetch_add(1, std::memory_order_relaxed));
        }
        static inline unsigned long long getSequenceNumber()
        {
            // 4 bits were reserved as the nodeID
            return (g_ullSequenceNumber.fetch_add(1, std::memory_order_relaxed) + g_ullNodeId);
        }
        static inline unsigned long long getNumber(std::atomic<unsigned long long>& ullNumber)
        {
            // 4 bits were reserved as the nodeID
            return (ullNumber.fetch_add(1, std::memory_order_relaxed) + g_ullNodeId);
        }
        static inline auto double2int(double in)
        {
            return in > 0 ? (in + 0.5) : (in - 0.5);
        }
        static inline double doubleAccuracy(double in, int n)
        {
            long l = pow(10, n);
            long long ll = in * l;
            double d = (double)ll / l;
            return d;
        }
        template <typename ValueType>
        static inline void getJsonValue(ValueType& value, const nlohmann::json& jsonData, const std::string& strKey)
        {
            if (jsonData.contains(strKey))
            {
                value = jsonData[strKey].get<ValueType>();
            }
        }
        static inline unsigned int crc32b(unsigned char *message) {
            int i, j;
            unsigned int byte, crc, mask;

            i = 0;
            crc = 0xFFFFFFFF;
            while (message[i] != 0) {
                byte = message[i];            // Get next byte.
                crc = crc ^ byte;
                for (j = 7; j >= 0; j--) {    // Do eight times.
                    mask = -(crc & 1);
                    crc = (crc >> 1) ^ (0xEDB88320 & mask);
                }
                i = i + 1;
            }
            return ~crc;
        }
        static inline void stringSplit(std::vector<std::string>& vecStr, const std::string& strSrc, const std::string& strSplit) {
            std::regex regexSplit(strSplit);
            std::sregex_token_iterator tokens(strSrc.begin(), strSrc.end(), regexSplit, -1);
            std::sregex_token_iterator end;
            for (; tokens != end; ++tokens) {
                vecStr.push_back(*tokens);
            }
        }

    };
}
#endif //MATCHING_ENGINE_UTILS_H
