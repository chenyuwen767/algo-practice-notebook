#pragma once

#include <filesystem>
#include <string>
#include <vector>

// 一次练习记录：同一天、同一道题可能练多次，所以单独存数组而不是只存一个完成时间。
struct Attempt {
    std::string date = {};     // ISO 日期，例如 2026-11-07
    int minutes = 0;           // 本次耗时（分钟），0 表示没记
    std::string outcome = {};  // ac / rejected / unfinished / reviewed
    std::string note = {};
};

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
    std::vector<Attempt> attempts = {};
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
