/**
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef APP_H
#define APP_H
#pragma once

#include "ThreadMgr.h"

namespace {
    int g_MainThreadId = 0;
}

typedef int (*MsgProcess)(uint32_t msgId, std::shared_ptr<void> msgData, void* userData);

class App {
public:
    /**
    * @brief Constructor
    */
    App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /**
    * @brief Destructor
    */
    ~App();

    /**
     * @brief Get the single instance of App
     * @return Instance of App
     */
    static App& GetInstance()
    {
        static App instance;
        return instance;
    }

    /**
     * @brief Create one app thread
     * @return Result of create thread
     */
    int CreateThread(Thread* thInst, const std::string& instName,
                            aclrtContext context, aclrtRunMode runMode, const uint32_t msgQueueSize);
    int Start(std::vector<ThreadParam>& threadParamTbl);
    void Wait();
    void Wait(MsgProcess msgProcess, void* param);
    int GetThreadIdByName(const std::string& threadName);
    Error SendMessage(int dest, int msgId, std::shared_ptr<void> data);
    void WaitEnd()
    {
        waitEnd_ = true;
    }
    void Exit();

private:
    Error Init();
    int CreateThreadMgr(Thread* thInst, const std::string& instName,
                               aclrtContext context, aclrtRunMode runMode, const uint32_t msgQueueSize);
    bool CheckThreadAbnormal();
    bool CheckThreadNameUnique(const std::string& threadName);
    void ReleaseThreads();

private:
    bool isReleased_;
    bool waitEnd_;
    std::vector<ThreadMgr*> threadList_;
};

App& CreateAppInstance();
App& GetAppInstance();
Error SendMessage(int dest, int msgId, std::shared_ptr<void> data);
int GetThreadIdByName(const std::string& threadName);
#endif