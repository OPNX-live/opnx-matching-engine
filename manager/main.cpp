//
// Created by Bob   on 2021/12/8.
//

#include <string>
#include <fstream>
#include "signal.h"

#include "manager.h"
#include "log.h"
#include "utils.h"
#include "global.h"


// init global variables of global.h
unsigned long long g_ullPerpMarketId = 0;
long long g_llPerpMarkPrice = 0;
unsigned long long g_ullRepoMarketId = 0;

// init global variables of utils.h
std::atomic<unsigned long long> g_ullSortId(static_cast<unsigned long long>(time(nullptr)) * 10000);
std::atomic<unsigned long long> g_ullMatchId(static_cast<unsigned long long>(time(nullptr)) * 10000);
std::atomic<unsigned long long> g_ullSequenceNumber(static_cast<unsigned long long>(time(nullptr)) * 10000);
unsigned long long g_ullNodeId = 0;

Manager* g_pManager = nullptr;

CallBackPulsarLog pulsarLog = [](const std::string& strLog) {
    if (nullptr != g_pManager)
    {
        g_pManager->sendPulsarLog(strLog);
    }
};
OPNX::Log cfLog(OPNX::Log::DEBUG, pulsarLog);

void signal_handler( int signalNum ) {

    cfLog.printInfo() << "signal_handler signaled is " << signalNum << std::endl;
    if (SIGQUIT == signalNum || SIGABRT == signalNum || SIGTERM == signalNum) {
        if (nullptr != g_pManager)
        {
            g_pManager->exit();
            g_pManager = nullptr;
        }
    }
    else if (SIGUSR1 == signalNum) {
        if (nullptr != g_pManager)
        {
            g_pManager->openPulsarLog();
            cfLog.info() << "---enablePulsarLog---" << std::endl;
        }

    }
    else if (SIGUSR2 == signalNum) {
        if (nullptr != g_pManager)
        {
            g_pManager->closedPulsarLog();
            cfLog.info() << "---disablePulsarLog---" << std::endl;
        }

    }
    cfLog.printInfo() << "signal_handler is end" << std::endl;
}

int main(int iArgc, char** pszArgv) {

    signal(SIGQUIT, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    std::string strServiceUrl = "";
    std::string strReferencePair = "";
    nlohmann::json  jsonConfig;
    std::string strFile("./config.json");
    std::ifstream ifst(strFile);
    ifst >> jsonConfig;
    ifst.close();
    if (3 > iArgc)
    {
        if (jsonConfig.contains("StartupParameters"))
        {
            if (jsonConfig["StartupParameters"].contains("ServiceUrl"))
            {
                strServiceUrl = jsonConfig["StartupParameters"]["ServiceUrl"].get<std::string>();
            }
            else
            {
                cfLog.fatal() << pszArgv[0] << " argv error, usage: pulsarUrl ReferencePair " << std::endl;
                return -1;
            }
            if (jsonConfig["StartupParameters"].contains("ReferencePair"))
            {
                strReferencePair = jsonConfig["StartupParameters"]["ReferencePair"].get<std::string>();
            }
            else
            {
                cfLog.fatal() << pszArgv[0] << " argv error, usage: pulsarUrl ReferencePair " << std::endl;
                return -1;
            }
        }
        else
        {
            cfLog.fatal() << pszArgv[0] << " argv error, usage: pulsarUrl ReferencePair " << std::endl;
            return -1;
        }
    }
    else
    {
        strServiceUrl = pszArgv[1];
        strReferencePair = pszArgv[2];
    }
    bool enablePulsarLog = false;
    if (4 <= iArgc && 1 == atoi(pszArgv[3]))  // Enable PulsarLog through startup parameters
    {
        enablePulsarLog = true;
    }

    cfLog.warn() << "ME is Running, ReferencePair: " << strReferencePair << ", ServiceUrl: " << strServiceUrl << std::endl;

    Manager manager(strServiceUrl, strReferencePair, jsonConfig);
    g_pManager = &manager;
    manager.run(enablePulsarLog);
    g_pManager = nullptr;

    cfLog.warn() << "main ME is exited, ReferencePair: " << strReferencePair << ", ServiceUrl: " << strServiceUrl << std::endl;
    return 0;
}
