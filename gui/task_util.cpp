#include "task_util.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>

#include "task_table_model.h"

namespace taskui {

QString taskText(const std::string& value) {
    return QString::fromUtf8(value.c_str());
}

std::string toTaskText(const QString& value) {
    return value.toUtf8().toStdString();
}

QString tagsText(const std::vector<std::string>& tags) {
    return taskTagsText(tags);
}

std::vector<std::string> toTaskTags(const QString& text) {
    QString normalized = text;
    normalized.replace("，", ",");
    normalized.replace("；", ";");
    normalized.replace(";", ",");

    std::vector<std::string> tags;
    for (const QString& part : normalized.split(',', Qt::SkipEmptyParts)) {
        const QString tag = part.trimmed();
        if (!tag.isEmpty()) {
            tags.push_back(toTaskText(tag));
        }
    }
    return tags;
}

std::string nowIso() {
    return QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
}

std::string newTaskId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
}

void ensureTaskIds(std::vector<Task>& tasks) {
    for (Task& task : tasks) {
        if (task.id.empty()) {
            task.id = newTaskId();
        }
    }
}

QString displayPath(const std::filesystem::path& path) {
    return QString::fromStdWString(path.wstring());
}

QString platformLabel(const QString& platform) {
    const QString lower = platform.trimmed().toLower();
    if (lower == "codeforces") {
        return "Codeforces";
    }
    if (lower == "luogu") {
        return "洛谷";
    }
    if (lower.isEmpty()) {
        return "未标注";
    }
    return platform;
}

QString optionalNumber(const QJsonValue& value) {
    return value.isDouble() ? QString::number(value.toInt()) : QString("未公开");
}

app_paths::DataFileInfo resolveDataFileInfo() {
    QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty()) {
        directory = QCoreApplication::applicationDirPath();
    }
    return app_paths::resolveDataFileInfo(
        QCoreApplication::arguments(),
        QCoreApplication::applicationDirPath(),
        QDir::currentPath(),
        directory);
}

} // namespace taskui
