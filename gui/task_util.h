#pragma once

#include <QJsonValue>
#include <QString>

#include <filesystem>
#include <string>
#include <vector>

#include "app_paths.h"
#include "task_store.h"

namespace taskui {

// std::string（UTF-8）与 QString 的转换。数据层统一用 std::string，界面层统一用 QString。
QString taskText(const std::string& value);
std::string toTaskText(const QString& value);

QString tagsText(const std::vector<std::string>& tags);
std::vector<std::string> toTaskTags(const QString& text);

std::string nowIso();
std::string newTaskId();
void ensureTaskIds(std::vector<Task>& tasks);

QString displayPath(const std::filesystem::path& path);
QString platformLabel(const QString& platform);
QString optionalNumber(const QJsonValue& value);

// 解析任务数据文件位置（AppData -> 程序目录 -> 临时目录的降级在 app_paths 里）。
app_paths::DataFileInfo resolveDataFileInfo();

} // namespace taskui
