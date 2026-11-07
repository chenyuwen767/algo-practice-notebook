#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "task_store.h"

struct JsonLoadResult {
    std::vector<Task> tasks;
    bool ok = true;
    std::string error;
    std::vector<std::string> warnings;
    // 读到的文件版本低于当前版本（v5），下次保存会写成新版本。
    bool migrated = false;
};

JsonLoadResult loadTasksJson(const std::filesystem::path& fileName);
bool saveTasksJson(const std::vector<Task>& tasks,
                   const std::filesystem::path& fileName,
                   std::string& error);
