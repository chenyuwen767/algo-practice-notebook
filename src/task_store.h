#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct Task {
    std::string title;
    std::string category;
    std::string difficulty;
    bool completed = false;
    std::vector<std::string> tags = {};
    std::string notes = {};
    std::string id = {};
    std::string platform = {};
    std::string problemId = {};
    std::string createdAt = {};
    std::string completedAt = {};
    // 归一化难度（与 Codeforces rating 同刻度，0 = 未标注），由 difficulty.cpp 计算。
    // 放在末尾，避免打乱既有 Task 聚合初始化的字段顺序。
    int difficultyRank = 0;
};

struct LoadResult {
    std::vector<Task> tasks;
    bool ok = true;
    std::string error;
    std::vector<std::string> warnings;
};

LoadResult loadTasksChecked(const std::filesystem::path& fileName);
std::vector<Task> loadTasks(const std::filesystem::path& fileName);
bool saveTasksChecked(const std::vector<Task>& tasks,
                      const std::filesystem::path& fileName,
                      std::string& error);
bool saveTasks(const std::vector<Task>& tasks, const std::filesystem::path& fileName);
