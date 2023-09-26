
#ifndef TYPE_H
#define TYPE_H
#pragma once

#include <unistd.h>
#include <string>
#include <memory>

enum MemoryType
{
    MEMORY_NORMAL = 0,
    MEMORY_HOST,
    MEMORY_DEVICE,
    MEMORY_DVPP,
    MEMORY_INVALID_TYPE
};

enum CopyDirection
{
    TO_DEVICE = 0,
    TO_HOST,
    INVALID_COPY_DIRECT
};

enum CameraId
{
    CAMERA_ID_0 = 0,
    CAMERA_ID_1,
    CAMERA_ID_INVALID,
};

enum VencStatus
{
    STATUS_VENC_INIT = 0,
    STATUS_VENC_WORK,
    STATUS_VENC_FINISH,
    STATUS_VENC_EXIT,
    STATUS_VENC_ERROR
};

typedef void *aclrtContext;

enum acldvppPixelFormat
{
    PIXEL_FORMAT_YUV_SEMIPLANAR_420 = 0

};

enum acldvppStreamFormat
{
    H264_MAIN_LEVEL = 0
};

enum aclrtRunMode {
    ACL_HOST = 0
};

struct VencConfig
{
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;
    std::string outFile;
    acldvppPixelFormat format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    acldvppStreamFormat enType = H264_MAIN_LEVEL;
    aclrtContext context = nullptr;
    aclrtRunMode runMode = ACL_HOST;
};

struct ImageData
{
    acldvppPixelFormat format;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t alignWidth = 0;
    uint32_t alignHeight = 0;
    uint32_t size = 0;
    std::shared_ptr<uint8_t> data = nullptr;
};

struct FrameData
{
    bool isFinished = false;
    uint32_t frameId = 0;
    uint32_t size = 0;
    void *data = nullptr;
};

struct Resolution
{
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Rect
{
    uint32_t ltX = 0;
    uint32_t ltY = 0;
    uint32_t rbX = 0;
    uint32_t rbY = 0;
};

struct BBox
{
    Rect rect;
    uint32_t score = 0;
    std::string text;
};

struct Message
{
    int dest;
    int msgId;
    std::shared_ptr<void> data = nullptr;
};

struct DataInfo
{
    void *data;
    uint32_t size;
};

struct InferenceOutput
{
    std::shared_ptr<void> data = nullptr;
    uint32_t size;
};

// struct ModelOutputInfo
// {
//     const char *name;
//     aclmdlIODims dims;
//     aclFormat format;
//     aclDataType dataType;
// };

#endif