#ifndef __OPNX_LOG_H__
#define __OPNX_LOG_H__

#include <chrono>
#include <ctime>
#include <iomanip>

#include <iostream>
#include <sstream>

using CallBackPulsarLog = std::function<void(const std::string&)>;

namespace OPNX {
    class LogBuf : public std::stringbuf {
        friend class LogStream;

    private:
        std::ostream *stream_ptr;
        CallBackPulsarLog* pulsarLog;

    private:
        explicit LogBuf(std::ostream *stream_ptr, CallBackPulsarLog *pulsarLog) noexcept: stream_ptr(stream_ptr), pulsarLog(pulsarLog) {}

    protected:
        int sync() override {
            auto ret = this->std::stringbuf::sync();
            if (stream_ptr) {
                auto str = this->str();
                stream_ptr->write(str.data(), str.size()).flush();

                if (pulsarLog) {
                    (*pulsarLog)(str);
                }
            }
            return ret;
        };

    };


    class LogStream : public std::ostream {
        friend class Log;

    private:
        LogBuf buf;

    private:
        explicit LogStream(std::ostream *stream_ptr, CallBackPulsarLog *pulsarLog, const char label[]) : std::ostream(stream_ptr ? &buf : nullptr), buf(stream_ptr, pulsarLog) {
            if (stream_ptr) {
                auto now = std::chrono::system_clock::now();
                std::time_t time = std::chrono::system_clock::to_time_t(now);
                std::tm tm;
                ::localtime_r(&time, &tm);
                *this << std::put_time(&tm, "%FT%T") << ','
                      << std::setfill('0') << std::setw(6)
                      << std::chrono::duration_cast<std::chrono::microseconds>(now - std::chrono::time_point_cast<std::chrono::seconds>(now)).count()
                      << std::setfill(' ') << "  " << std::setw(5) << label << "  ";
            }
        };

    };


    class Log {

    public:
        enum Level: unsigned char{
            SILENT = 0, FATAL, ERROR, WARN, INFO, DEBUG, TRACE
        };

    public:
        std::ostream *m_pLogStream;
        Log::Level m_level1;
        CallBackPulsarLog m_pPulsarLog;

    public:
        Log(CallBackPulsarLog &pulsarLog) : m_pLogStream(&std::clog), m_level1(INFO), m_pPulsarLog(pulsarLog) {}
        Log(Level level, CallBackPulsarLog &pulsarLog) : m_pLogStream(&std::clog), m_level1(level), m_pPulsarLog(pulsarLog) {}
        void setPulsarLog(CallBackPulsarLog &pPulsarLog) { m_pPulsarLog = pPulsarLog; }

    public:
        inline void setLogLevel(Level level) { m_level1 = level; }
        inline bool enabledLevel(Level level) { return m_level1 >= level ? true : false; }

        inline bool enabledTrace() { return m_level1 >= TRACE ? true : false; }
        inline bool enabledDebug() { return m_level1 >= DEBUG ? true : false; }
        inline bool enabledInfo() { return m_level1 >= INFO ? true : false; }
        inline bool enabledWarn() { return m_level1 >= WARN ? true : false; }
        inline bool enabledError() { return m_level1 >= ERROR ? true : false; }
        inline bool enabledFatal() { return m_level1 >= FATAL ? true : false; }

        LogStream trace() { return m_level1 >= TRACE ? LogStream(m_pLogStream, nullptr, "TRACE") : LogStream(nullptr, nullptr, "TRACE"); }
        LogStream debug() { return m_level1 >= DEBUG ? LogStream(m_pLogStream, nullptr, "DEBUG") : LogStream(nullptr, nullptr, "DEBUG"); }
        LogStream info() { return m_level1 >= INFO ? LogStream(m_pLogStream, &m_pPulsarLog, "INFO") : LogStream(nullptr, nullptr, "INFO"); }
        LogStream warn() { return m_level1 >= WARN ? LogStream(m_pLogStream, &m_pPulsarLog, "WARN") : LogStream(nullptr, nullptr, "WARN"); }
        LogStream error() { return m_level1 >= ERROR ? LogStream(m_pLogStream, &m_pPulsarLog, "ERROR") : LogStream(nullptr, nullptr, "ERROR"); }
        LogStream fatal() { return m_level1 >= FATAL ? LogStream(m_pLogStream, &m_pPulsarLog, "FATAL") : LogStream(nullptr, nullptr, "FATAL"); }
        LogStream printInfo() { return m_level1 >= FATAL ? LogStream(m_pLogStream, &m_pPulsarLog, "INFO") : LogStream(nullptr, nullptr, "INFO"); }

    };
}

extern OPNX::Log cfLog;   // Declare a global log object, please define it in the main.cpp file to ensure that the log object is available

namespace OPNX {
    class CInOutLog {
    public:
        CInOutLog(const char *szFlag) {
            m_strFlag = szFlag;
            cfLog.debug() << " CInOutLog: " << m_strFlag << " in " << std::endl;
        }
        CInOutLog(const std::string& strFlag) {
            m_strFlag = strFlag;
            cfLog.debug() << " CInOutLog: " << m_strFlag << " in " << std::endl;
        }

        ~CInOutLog() {
            cfLog.debug() << " CInOutLog: " << m_strFlag << " out " << std::endl;
        }

    private:
        std::string m_strFlag;
    };
}
#endif  // __OPNX_LOG_H__