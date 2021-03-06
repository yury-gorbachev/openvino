// Copyright (C) 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "utils.h"

#include <string>
#include <string.h>

std::string OS_PATH_JOIN(std::initializer_list<std::string> list) {
    if (!list.size())
        return "";
    std::string res = *list.begin();
    for (auto it = list.begin() + 1; it != list.end(); it++) {
        res += OS_SEP + *it;
    }
    return res;
}

std::string fileNameNoExt(const std::string &filepath) {
    auto pos = filepath.rfind('.');
    if (pos == std::string::npos) return filepath;
    return filepath.substr(0, pos);
}


static size_t parseLine(char* line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    size_t i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = (size_t)atoi(p);
    return i;
}

#ifdef _WIN32
size_t getVmSizeInKB() {
                // TODO rewrite for Virtual Memory
                PROCESS_MEMORY_COUNTERS pmc;
                pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
                GetProcessMemoryInfo(GetCurrentProcess(),&pmc, pmc.cb);
                return pmc.WorkingSetSize;
	    }
#else
size_t getVirtualMemoryInKB(char *name){
    FILE* file = fopen("/proc/self/status", "r");
    size_t result = 0;
    if (file != nullptr) {
        char line[128];

        while (fgets(line, 128, file) != NULL) {
            if (strncmp(line, name, strlen(name)) == 0) {
                result = parseLine(line);
                break;
            }
        }
        fclose(file);
    }
    return result;
}

size_t getVmSizeInKB() {return getVirtualMemoryInKB((char*) "VmSize:");}
size_t getVmPeakInKB() {return getVirtualMemoryInKB((char*) "VmPeak:");}
size_t getVmRSSInKB() {return getVirtualMemoryInKB((char*) "VmRSS:");}
size_t getVmHWMInKB() {return getVirtualMemoryInKB((char*) "VmHWM:");}

#endif

void auto_expand_env_vars(std::string &input) {
    const static std::string pattern1 = "${", pattern2 = "}";
    size_t pattern1_pos, pattern2_pos, envvar_start_pos, envvar_finish_pos;
    while ((pattern1_pos = input.find(pattern1)) != std::string::npos) {
        envvar_start_pos = pattern1_pos + pattern1.length();
        if ((pattern2_pos = input.find(pattern2)) != std::string::npos) {
            envvar_finish_pos = pattern2_pos - pattern2.length();
            const std::string envvar_name = input.substr(envvar_start_pos, envvar_finish_pos - envvar_start_pos + 1);
            const char *envvar_val = getenv(envvar_name.c_str());
            if (envvar_val == NULL)
                throw std::logic_error("Expected environment variable " + envvar_name + " is not set.");
            const std::string envvar_val_s(envvar_val);
            input.replace(pattern1_pos, pattern2_pos - pattern1_pos + 1, envvar_val_s);
        }
    }
}
std::string expand_env_vars(const std::string &input) {
    std::string _input = input;
    auto_expand_env_vars(_input);
    return _input;
}
