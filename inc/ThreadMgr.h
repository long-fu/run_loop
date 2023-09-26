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

* File ThreadMgr.h
* Description: handle ThreadMgr operations
*/
#ifndef THREADMGR_H
#define THREADMGR_H
#pragma once
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include "Utils.h"
#include "ThreadSafeQueue.h"
#include "Thread.h"

enum ThreadStatus {
    THREAD_READY = 0,
    THREAD_RUNNING = 1,
    THREAD_EXITING = 2,
    THREAD_EXITED = 3,
    THREAD_ERROR = 4,
};

class ThreadMgr {
public:
    ThreadMgr(Thread* userThreadInstance,
                     const std::string& threadName, const uint32_t msgQueueSize);
    ~ThreadMgr();
    // Thread function
    static void ThreadEntry(void* data);
    Thread* GetUserInstance()
    {
        return this->userInstance_;
    }
    const std::string& GetThreadName()
    {
        return name_;
    }
    // Send Message data to the queue
    Error PushMsgToQueue(std::shared_ptr<Message>& pMessage);
    // Get Message data from the queue
    std::shared_ptr<Message> PopMsgFromQueue()
    {
        return this->msgQueue_.Pop();
    }
    void CreateThread();
    void SetStatus(ThreadStatus status)
    {
        status_ = status;
    }
    ThreadStatus GetStatus()
    {
        return status_;
    }
    Error WaitThreadInitEnd();
 
public:
    bool isExit_;
    ThreadStatus status_;
    Thread* userInstance_;
    std::string name_;
    ThreadSafeQueue<std::shared_ptr<Message>> msgQueue_;
};
#endif