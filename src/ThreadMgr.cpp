/**
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

* File ThreadMgr.cpp
* Description: handle file operations
*/
#include "ThreadMgr.h"
#include "Utils.h"
using namespace std;
namespace {
    const uint32_t kWait10Milliseconds = 10000;
    const uint32_t kWaitThreadStart = 1000;
}

ThreadMgr::ThreadMgr(Thread* userThreadInstance,
    const string& threadName, const uint32_t msgQueueSize):isExit_(false),
    status_(THREAD_READY), userInstance_(userThreadInstance),
    name_(threadName), msgQueue_(msgQueueSize)
{
}

ThreadMgr::~ThreadMgr()
{
    userInstance_ = nullptr;
    while (!msgQueue_.Empty()) {
        msgQueue_.Pop();
    }
}

void ThreadMgr::CreateThread()
{
    // 创建线程
    thread engine(&ThreadMgr::ThreadEntry, (void *)this);
    engine.detach();
}

void ThreadMgr::ThreadEntry(void* arg)
{
    ThreadMgr* thMgr = (ThreadMgr*)arg;
    Thread* userInstance = thMgr->GetUserInstance();
    if (userInstance == nullptr) {
        LOG_ERROR(" thread exit for user thread instance is null");
        return;
    }

    string& instName = userInstance->SelfInstanceName();
    aclrtContext context = userInstance->GetContext();
    // aclError aclRet = aclrtSetCurrentContext(context);
    // if (aclRet != ACL_SUCCESS) {
    //     LOG_ERROR("Thread %s set context failed, error: %d",
    //                       instName.c_str(), aclRet);
    //     return;
    // }

    int ret = userInstance->Init();
    if (ret) {
        LOG_ERROR("Thread %s init error %d, thread exit",
                          instName.c_str(), ret);
        thMgr->SetStatus(THREAD_ERROR);
        return;
    }

    thMgr->SetStatus(THREAD_RUNNING);
    while (THREAD_RUNNING == thMgr->GetStatus()) {
        // get data from queue
        shared_ptr<Message> msg = thMgr->PopMsgFromQueue();
        if (msg == nullptr) {
            usleep(kWait10Milliseconds);
            continue;
        }
        // call function to process thread msg
        ret = userInstance->Process(msg->msgId, msg->data);
        msg->data = nullptr;
        if (ret) {
            LOG_ERROR("Thread %s process function return "
                              "error %d, thread exit", instName.c_str(), ret);
            thMgr->SetStatus(THREAD_ERROR);
            return;
        }
        usleep(0);
    }
    thMgr->SetStatus(THREAD_EXITED);

    return;
}

Error ThreadMgr::WaitThreadInitEnd()
{
    while (true) {
        if (status_ == THREAD_RUNNING) {
            break;
        } else if (status_ > THREAD_RUNNING) {
            string& instName = userInstance_->SelfInstanceName();
            LOG_ERROR("Thread instance %s status change to %d, "
                              "app start failed", instName.c_str(), status_);
            return ERROR_START_THREAD;
        } else {
            usleep(kWaitThreadStart);
        }
    }

    return OK;
}

Error ThreadMgr::PushMsgToQueue(shared_ptr<Message>& pMessage)
{
    if (status_ != THREAD_RUNNING) {
        LOG_ERROR("Thread instance %s status(%d) is invalid, "
                          "can not reveive message", name_.c_str(), status_);
        return ERROR_THREAD_ABNORMAL;
    }
    return msgQueue_.Push(pMessage)? OK : ERROR_ENQUEUE;
}