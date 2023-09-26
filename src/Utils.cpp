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

* File utils.cpp
* Description: handle file operations
*/
#include <map>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <regex>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "Utils.h"


using namespace std;

#define COMMENT_CHAR '#'
#define EQUALS_CHAR  '='
#define BLANK_SPACE_CHAR ' '
#define TABLE_CHAR '\t'

namespace {
const std::string kImagePathSeparator = ",";
const int kStatSuccess = 0;
const std::string kFileSperator = "/";
const std::string kPathSeparator = "/";
// output image prefix
const std::string kOutputFilePrefix = "out_";

const string kRegexIpAddr =
    "^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[0-9])\\."
    "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
    "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
    "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)"
    ":([1-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|"
    "6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])$";

// regex for verify video file name
const string kRegexVideoFile = "^.+\\.(mp4|h264|h265)$";

// regex for verify RTSP rtsp://ip:port/channelname
const string kRegexRtsp = "^rtsp://.*";
}

bool IsDigitStr(const string& str)
{
    for (uint32_t i = 0; i<str.size(); i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

bool IsPathExist(const string &path)
{
    ifstream file(path);
    if (!file) {
        return false;
    }
    return true;
}

bool IsVideoFile(const string& str)
{
    regex regexVideoFile(kRegexVideoFile.c_str());
    return regex_match(str, regexVideoFile);
}

bool IsRtspAddr(const string &str)
{
    regex regexRtspAddress(kRegexRtsp.c_str());

    return regex_match(str, regexRtspAddress);
}

bool IsIpAddrWithPort(const string& addrStr)
{
    regex regexIpAddr(kRegexIpAddr.c_str());

    return regex_match(addrStr, regexIpAddr);
}

void ParseIpAddr(string& ip, string& port, const string& addr)
{
    string::size_type pos = addr.find(":");
    
    ip.assign(addr.substr(0, pos));
    port.assign(addr.substr(pos + 1));
}

bool IsDirectory(const string &path)
{
    // get path stat
    struct stat buf;
    if (stat(path.c_str(), &buf) != kStatSuccess) {
        return false;
    }

    // check
    return S_ISDIR(buf.st_mode);
}

void SplitPath(const string &path, vector<string> &pathVec)
{
    char *charPath = const_cast<char*>(path.c_str());
    const char *charSplit = kImagePathSeparator.c_str();
    char *imageFile = strtok(charPath, charSplit);
    while (imageFile) {
        pathVec.emplace_back(imageFile);
        imageFile = strtok(nullptr, charSplit);
    }
}

void GetPathFiles(const string &path, vector<string> &fileVec)
{
    struct dirent *direntPtr = nullptr;
    DIR *dir = nullptr;
    if (IsDirectory(path)) {
        dir = opendir(path.c_str());
        while ((direntPtr = readdir(dir)) != nullptr) {
            // skip . and ..
            if (direntPtr->d_name[0] == '.') {
            continue;
            }

            // file path
            string fullPath = path + kPathSeparator + direntPtr->d_name;
            // directory need recursion
            if (IsDirectory(fullPath)) {
                GetPathFiles(fullPath, fileVec);
            } else {
                // put file
                fileVec.emplace_back(fullPath);
            }
        }
    } else {
        fileVec.emplace_back(path);
    }
}

void GetAllFiles(const string &pathList, vector<string> &fileVec)
{
    // split file path
    vector<string> pathVec;
    SplitPath(pathList, pathVec);

    for (string everyPath : pathVec) {
        // check path exist or not
        if (!IsPathExist(pathList)) {
            LOG_ERROR("Failed to deal path=%s. Reason: not exist or can not access.",
                everyPath.c_str());
            continue;
        }
        // get files in path and sub-path
        GetPathFiles(everyPath, fileVec);
    }
}

Error ReadBinFile(const string& fileName, void*& data, uint32_t& size)
{
    struct stat sBuf;
    int fileStatus = stat(fileName.data(), &sBuf);
    if (fileStatus == -1) {
        LOG_ERROR("failed to get file");
        return ERROR_ACCESS_FILE;
    }
    if (S_ISREG(sBuf.st_mode) == 0) {
        LOG_ERROR("%s is not a file, please enter a file",
                          fileName.c_str());
        return ERROR_INVALID_FILE;
    }
    std::ifstream binFile(fileName, std::ifstream::binary);
    if (binFile.is_open() == false) {
        LOG_ERROR("open file %s failed", fileName.c_str());
        return ERROR_OPEN_FILE;
    }

    binFile.seekg(0, binFile.end);
    uint32_t binFileBufferLen = binFile.tellg();
    if (binFileBufferLen == 0) {
        LOG_ERROR("binfile is empty, filename is %s", fileName.c_str());
        binFile.close();
        return ERROR_INVALID_FILE;
    }

    binFile.seekg(0, binFile.beg);

    uint8_t* binFileBufferData = new(std::nothrow) uint8_t[binFileBufferLen];
    if (binFileBufferData == nullptr) {
        LOG_ERROR("malloc binFileBufferData failed");
        binFile.close();
        return ERROR_MALLOC;
    }
    binFile.read((char *)binFileBufferData, binFileBufferLen);
    binFile.close();

    data = binFileBufferData;
    size = binFileBufferLen;

    return OK;
}

void SaveBinFile(const string& filename, const void* data, uint32_t size)
{
    FILE *outFileFp = fopen(filename.c_str(), "wb+");
    if (outFileFp == nullptr) {
        LOG_ERROR("Save file %s failed for open error", filename.c_str());
        return;
    }
    fwrite(data, 1, size, outFileFp);

    fflush(outFileFp);
    fclose(outFileFp);
}

bool IsSpace(char c)
{
    return (c == BLANK_SPACE_CHAR || c == TABLE_CHAR);
}

void Trim(string& str)
{
    if (str.empty()) {
        return;
    }
    uint32_t i, startPos, endPos;
    for (i = 0; i < str.size(); ++i) {
        if (!IsSpace(str[i])) {
            break;
        }
    }
    if (i == str.size()) { // is all blank space
        str = "";
        return;
    }

    startPos = i;
    
    for (i = str.size() - 1; i >= 0; --i) {
        if (!IsSpace(str[i])) {
            break;
        }
    }
    endPos = i;

    str = str.substr(startPos, endPos - startPos + 1);
}

bool AnalyseLine(const string & line, string & key, string & value)
{
    if (line.empty()) {
        return false;
    }

    int startPos = 0;
    int endPos = line.size() - 1;
    int pos = 0;
    if ((pos = line.find(COMMENT_CHAR)) != -1) {
        if (pos == 0) {  // the first charactor is #
            return false;
        }
        endPos = pos - 1;
    }
    string new_line = line.substr(startPos, startPos + 1 - endPos);  // delete comment
    if ((pos = new_line.find(EQUALS_CHAR)) == -1) {
        return false;  // has no =
    }

    key = new_line.substr(0, pos);
    value = new_line.substr(pos + 1, endPos + 1- (pos + 1));

    Trim(key);
    if (key.empty()) {
        return false;
    }
    Trim(value);
    return true;
}

bool ReadConfig(map<string, string>& config, const char* configFile)
{
    config.clear();
    ifstream infile(configFile);
    if (!infile) {
        cout << "file open error" << endl;
        return false;
    }
    string line, key, value;
    while (getline(infile, line)) {
        if (AnalyseLine(line, key, value)) {
            config[key] = value;
        }
    }
    infile.close();
    return true;
}

void PrintConfig(const map<string, string>& config)
{
    map<string, string>::const_iterator mIter = config.begin();
    for (; mIter != config.end(); ++mIter) {
        cout << mIter->first << "=" << mIter->second << endl;
    }
}
