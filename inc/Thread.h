
#ifndef THREAD_H
#define THREAD_H
#pragma once
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include "ThreadSafeQueue.h"
#include "Error.h"
#include "Type.h"

#define INVALID_INSTANCE_ID (-1)
class Thread {
public:
    Thread();
    virtual ~Thread() {};
    virtual int Init()
    {
        return OK;
    };
    virtual int Process(int msgId, std::shared_ptr<void> msgData) = 0;
    int SelfInstanceId()
    {
        return instanceId_;
    }
    std::string& SelfInstanceName()
    {
        return instanceName_;
    }
    aclrtContext GetContext()
    {
        return context_;
    }
    aclrtRunMode GetRunMode()
    {
        return runMode_;
    }
    Error BaseConfig(int instanceId, const std::string& threadName,
                            aclrtContext context, aclrtRunMode runMode);
private:
    aclrtContext context_;
    aclrtRunMode runMode_;
    int instanceId_;
    std::string instanceName_;
    bool baseConfiged_;
    bool isExit_;
};

struct ThreadParam {
    Thread* threadInst = nullptr;
    std::string threadInstName = "";
    aclrtContext context = nullptr;
    aclrtRunMode runMode = ACL_HOST;
    int threadInstId = INVALID_INSTANCE_ID;
    uint32_t queueSize = 256;
};
#endif