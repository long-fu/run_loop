/**
* @file sample_process.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "App.h"
#include "ThreadMgr.h"

using namespace std;
namespace {
const uint32_t kWaitInterval = 10000;
const uint32_t kThreadExitRetry = 3;
}

App::App():isReleased_(false), waitEnd_(false)
{
    Init();
}

App::~App()
{
    ReleaseThreads();
}

Error App::Init()
{
    const uint32_t msgQueueSize = 256;
    ThreadMgr* thMgr = new ThreadMgr(nullptr, "main", msgQueueSize);
    threadList_.push_back(thMgr);
    thMgr->SetStatus(THREAD_RUNNING);
    return OK;
}

int App::CreateThread(Thread* thInst, const string& instName,
                                    aclrtContext context, aclrtRunMode runMode, const uint32_t msgQueueSize)
{
    int instId = CreateThreadMgr(thInst, instName, context, runMode, msgQueueSize);
    if (instId == INVALID_INSTANCE_ID) {
        LOG_ERROR("Add thread instance %s failed", instName.c_str());
        return INVALID_INSTANCE_ID;
    }

    threadList_[instId]->CreateThread();
    Error ret = threadList_[instId]->WaitThreadInitEnd();
    if (ret != OK) {
        LOG_ERROR("Create thread failed, error %d", ret);
        return INVALID_INSTANCE_ID;
    }

    return instId;
}

int App::CreateThreadMgr(Thread* thInst, const string& instName,
                                       aclrtContext context, aclrtRunMode runMode, const uint32_t msgQueueSize)
{
    if (!CheckThreadNameUnique(instName)) {
        LOG_ERROR("The thread instance name is not unique");
        return INVALID_INSTANCE_ID;
    }

    int instId = threadList_.size();
    Error ret = thInst->BaseConfig(instId, instName, context, runMode);
    if (ret != OK) {
        LOG_ERROR("Create thread instance failed for error %d", ret);
        return INVALID_INSTANCE_ID;
    }

    ThreadMgr* thMgr = new ThreadMgr(thInst, instName, msgQueueSize);
    threadList_.push_back(thMgr);

    return instId;
}

bool App::CheckThreadNameUnique(const string& threadName)
{
    if (threadName.size() == 0) {
        return true;
    }

    for (size_t i = 0; i < threadList_.size(); i++) {
        if (threadName == threadList_[i]->GetThreadName()) {
            return false;
        }
    }

    return true;
}

int App::Start(vector<ThreadParam>& threadParamTbl)
{
    for (size_t i = 0; i < threadParamTbl.size(); i++) {
        int instId = CreateThreadMgr(threadParamTbl[i].threadInst,
                                            threadParamTbl[i].threadInstName,
                                            threadParamTbl[i].context,
                                            threadParamTbl[i].runMode,
                                            threadParamTbl[i].queueSize);
        if (instId == INVALID_INSTANCE_ID) {
            LOG_ERROR("Create thread instance failed");
            return ERROR;
        }
        threadParamTbl[i].threadInstId = instId;
    }
    // Note:The instance id must generate first, then create thread,
    // for the user thread get other thread instance id in Init function
    for (size_t i = 0; i < threadParamTbl.size(); i++) {
        threadList_[threadParamTbl[i].threadInstId]->CreateThread();
    }

    for (size_t i = 0; i < threadParamTbl.size(); i++) {
        int instId = threadParamTbl[i].threadInstId;
        Error ret = threadList_[instId]->WaitThreadInitEnd();
        if (ret != OK) {
            LOG_ERROR("Create thread %s failed, error %d",
                              threadParamTbl[i].threadInstName.c_str(), ret);
            return ret;
        }
    }
    return OK;
}

int App::GetThreadIdByName(const string& threadName)
{
    if (threadName.empty()) {
        LOG_ERROR("search name is empty");
        return INVALID_INSTANCE_ID;
    }

    for (uint32_t i = 0; i < threadList_.size(); i++) {
        if (threadList_[i]->GetThreadName() == threadName) {
            return i;
        }
    }
    
    return INVALID_INSTANCE_ID;
}

Error App::SendMessage(int dest, int msgId, shared_ptr<void> data)
{
    if ((uint32_t)dest > threadList_.size()) {
        LOG_ERROR("Send message to %d failed for thread not exist", dest);
        return ERROR_DEST_INVALID;
    }

    shared_ptr<Message> pMessage = make_shared<Message>();
    pMessage->dest = dest;
    pMessage->msgId = msgId;
    pMessage->data = data;

    return threadList_[dest]->PushMsgToQueue(pMessage);
}

void App::Wait()
{
    while (true) {
        usleep(kWaitInterval);
        if (waitEnd_) break;
    }
    threadList_[g_MainThreadId]->SetStatus(THREAD_EXITED);
}

bool App::CheckThreadAbnormal()
{
    for (size_t i = 0; i < threadList_.size(); i++) {
        if (threadList_[i]->GetStatus() == THREAD_ERROR) {
            return true;
        }
    }

    return false;
}

void App::Wait(MsgProcess msgProcess, void* param)
{
    ThreadMgr* mainMgr = threadList_[0];

    if (mainMgr == nullptr) {
        LOG_ERROR(" app wait exit for message process function is nullptr");
        return;
    }

    while (true) {
        if (waitEnd_) break;

        shared_ptr<Message> msg = mainMgr->PopMsgFromQueue();
        if (msg == nullptr) {
            usleep(kWaitInterval);
            continue;
        }
        int ret = msgProcess(msg->msgId, msg->data, param);
        if (ret) {
            LOG_ERROR(" app exit for message %d process error:%d", msg->msgId, ret);
            break;
        }
    }
    threadList_[g_MainThreadId]->SetStatus(THREAD_EXITED);
}

void App::Exit()
{
    ReleaseThreads();
}

void App::ReleaseThreads()
{
    if (isReleased_) return;
    threadList_[g_MainThreadId]->SetStatus(THREAD_EXITED);

    for (uint32_t i = 1; i < threadList_.size(); i++) {
        if ((threadList_[i] != nullptr) &&
            (threadList_[i]->GetStatus() == THREAD_RUNNING))
             threadList_[i]->SetStatus(THREAD_EXITING);
    }

    int retry = kThreadExitRetry;
    while (retry >= 0) {
        bool exitFinish = true;
        for (uint32_t i = 0; i < threadList_.size(); i++) {
            if (threadList_[i] == nullptr)
                continue;
            if (threadList_[i]->GetStatus() > THREAD_EXITING) {
                delete threadList_[i];
                threadList_[i] = nullptr;
                LOG_INFO(" thread %d released", i);
            } else {
                exitFinish = false;
            }
        }

        if (exitFinish)
            break;

        sleep(1);
        retry--;
    }
    isReleased_ = true;
}

App& CreateAppInstance()
{
    return App::GetInstance();
}

App& GetAppInstance()
{
    return App::GetInstance();
}

Error SendMessage(int dest, int msgId, shared_ptr<void> data)
{
    App& app = App::GetInstance();
    return app.SendMessage(dest, msgId, data);
}

int GetThreadIdByName(const string& threadName)
{
    App& app = App::GetInstance();
    return app.GetThreadIdByName(threadName);
}
