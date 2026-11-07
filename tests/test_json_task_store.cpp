#include <QtTest>

#include <QFile>

#include "json_task_store.h"
#include "temp_dir.h"

namespace {

std::filesystem::path jsonPath(const TempDir& directory, const QString& name = "tasks.json") {
    return std::filesystem::path(directory.filePath(name).toStdWString());
}

QString textOf(const std::string& value) {
    return QString::fromUtf8(value.c_str());
}

bool writeFile(const QString& path, const QByteArray& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    return file.write(content) == content.size();
}

} // namespace

class JsonTaskStoreTest : public QObject {
    Q_OBJECT

private slots:
    void roundTripPreservesAllFields();
    void createsBackupOfPreviousJson();
    void reportsMalformedJson();
    void reportsInvalidEntriesWithoutDroppingValidOnes();
    void missingFileIsEmptyAndOk();
    void loadsVersion2WithoutNewFields();
    void migratesVersion4ToRankedDifficulty();
    void keepsRankWrittenByVersion5();
    void roundTripsAttempts();
    void migratesVersion5IfNoAttempts();
};

void JsonTaskStoreTest::roundTripPreservesAllFields() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);

    Task first;
    first.title = "P17633 最大异或对";
    first.category = "位运算";
    first.difficulty = "普及-";
    first.completed = true;
    first.tags = {"trie", "xor"};
    first.notes = "重做时先想 20 分钟\n第二行";
    first.id = "task-1";
    first.platform = "codeforces";
    first.problemId = "1900A";
    first.createdAt = "2026-10-30T10:00:00+08:00";
    first.completedAt = "2026-10-31T11:00:00+08:00";

    Task second;
    second.title = "CF 1900A";
    second.category = "构造";
    second.difficulty = "1600";
    second.completed = false;
    second.tags = {"constructive"};
    second.id = "task-2";

    std::string error;
    QVERIFY(saveTasksJson({first, second}, path, error));
    QVERIFY(error.empty());

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(loaded.ok);
    QVERIFY(loaded.warnings.empty());
    QCOMPARE(static_cast<int>(loaded.tasks.size()), 2);
    QCOMPARE(textOf(loaded.tasks[0].title), QString("P17633 最大异或对"));
    QCOMPARE(loaded.tasks[0].completed, true);
    QCOMPARE(loaded.tasks[0].tags.size(), std::size_t(2));
    QCOMPARE(textOf(loaded.tasks[0].tags[0]), QString("trie"));
    QCOMPARE(textOf(loaded.tasks[0].tags[1]), QString("xor"));
    QCOMPARE(textOf(loaded.tasks[0].notes), QString("重做时先想 20 分钟\n第二行"));
    QCOMPARE(textOf(loaded.tasks[0].id), QString("task-1"));
    QCOMPARE(textOf(loaded.tasks[0].platform), QString("codeforces"));
    QCOMPARE(textOf(loaded.tasks[0].problemId), QString("1900A"));
    QCOMPARE(textOf(loaded.tasks[0].createdAt), QString("2026-10-30T10:00:00+08:00"));
    QCOMPARE(textOf(loaded.tasks[0].completedAt), QString("2026-10-31T11:00:00+08:00"));
    QCOMPARE(textOf(loaded.tasks[1].id), QString("task-2"));
}

void JsonTaskStoreTest::createsBackupOfPreviousJson() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);

    std::string error;
    Task first;
    first.title = "first";
    QVERIFY(saveTasksJson({first}, path, error));

    Task second;
    second.title = "second";
    QVERIFY(saveTasksJson({second}, path, error));

    auto backupPath = path;
    backupPath += ".bak";
    const JsonLoadResult backup = loadTasksJson(backupPath);
    QVERIFY(backup.ok);
    QCOMPARE(static_cast<int>(backup.tasks.size()), 1);
    QCOMPARE(textOf(backup.tasks[0].title), QString("first"));

    const JsonLoadResult current = loadTasksJson(path);
    QVERIFY(current.ok);
    QCOMPARE(textOf(current.tasks[0].title), QString("second"));
}

void JsonTaskStoreTest::reportsMalformedJson() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);
    QVERIFY(writeFile(directory.filePath("tasks.json"), "{ this is not json"));

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(!loaded.ok);
    QVERIFY(!loaded.error.empty());
}

void JsonTaskStoreTest::reportsInvalidEntriesWithoutDroppingValidOnes() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);
    QVERIFY(writeFile(
        directory.filePath("tasks.json"),
        R"({"version":2,"tasks":[{"title":""},{"title":"ok","tags":"bad"},{"title":"good"}]})"));

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(loaded.ok);
    QCOMPARE(static_cast<int>(loaded.tasks.size()), 2);
    QVERIFY(loaded.warnings.size() >= 2);
    QCOMPARE(textOf(loaded.tasks[0].title), QString("ok"));
    QCOMPARE(textOf(loaded.tasks[1].title), QString("good"));
}

void JsonTaskStoreTest::missingFileIsEmptyAndOk() {
    TempDir directory;
    QVERIFY(directory.isValid());

    const JsonLoadResult loaded = loadTasksJson(jsonPath(directory, "missing.json"));
    QVERIFY(loaded.ok);
    QVERIFY(loaded.tasks.empty());
    QVERIFY(loaded.warnings.empty());
}

void JsonTaskStoreTest::loadsVersion2WithoutNewFields() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);
    QVERIFY(writeFile(directory.filePath("tasks.json"),
                      R"({"version":2,"tasks":[{"title":"legacy","completed":true}]})"));

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(loaded.ok);
    QCOMPARE(static_cast<int>(loaded.tasks.size()), 1);
    QCOMPARE(textOf(loaded.tasks[0].title), QString("legacy"));
    QVERIFY(loaded.tasks[0].platform.empty());
    QVERIFY(loaded.tasks[0].problemId.empty());
    QVERIFY(loaded.tasks[0].createdAt.empty());
    QVERIFY(loaded.tasks[0].completedAt.empty());
}

void JsonTaskStoreTest::migratesVersion4ToRankedDifficulty() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);
    QVERIFY(writeFile(directory.filePath("tasks.json"),
                      R"({"version":4,"tasks":[
                          {"title":"CF 1800","difficulty":"1800"},
                          {"title":"洛谷","difficulty":"普及-"},
                          {"title":"空难度","difficulty":""}]})"));

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(loaded.ok);
    QVERIFY(loaded.migrated);
    QCOMPARE(static_cast<int>(loaded.tasks.size()), 3);
    QCOMPARE(loaded.tasks[0].difficultyRank, 1800);
    QCOMPARE(loaded.tasks[1].difficultyRank, 1000);
    QCOMPARE(loaded.tasks[2].difficultyRank, 0);

    auto snapshot = path;
    snapshot += ".pre-v6";
    QVERIFY(std::filesystem::exists(snapshot));

    std::string error;
    QVERIFY(saveTasksJson(loaded.tasks, path, error));
    const JsonLoadResult again = loadTasksJson(path);
    QVERIFY(again.ok);
    QVERIFY(!again.migrated);
    QCOMPARE(again.tasks[1].difficultyRank, 1000);
    QCOMPARE(textOf(again.tasks[1].difficulty), QString("普及-"));
}

void JsonTaskStoreTest::keepsRankWrittenByVersion5() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);
    // 文本无法识别但调用方给了 rank 时，以写下来的 rank 为准。
    QVERIFY(writeFile(directory.filePath("tasks.json"),
                      R"({"version":5,"tasks":[{"title":"手工","difficulty":"自定义","difficultyRank":2100}]})"));

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(loaded.ok);
    // v5 只缺 attempts 字段，升到 v6 会在读取时补空数组，但 rank 必须原样保留。
    QVERIFY(loaded.migrated);
    QCOMPARE(loaded.tasks[0].difficultyRank, 2100);
}

void JsonTaskStoreTest::roundTripsAttempts() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);

    Task task;
    task.title = "带练习记录";
    Attempt first;
    first.date = "2026-01-01";
    first.minutes = 35;
    first.outcome = "rejected";
    first.note = "边界没想清楚";
    Attempt second;
    second.date = "2026-01-02";
    second.minutes = 20;
    second.outcome = "ac";
    task.attempts = {first, second};

    std::string error;
    QVERIFY(saveTasksJson({task}, path, error));

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(loaded.ok);
    QVERIFY(!loaded.migrated);
    QCOMPARE(static_cast<int>(loaded.tasks.size()), 1);
    const Task& back = loaded.tasks[0];
    QCOMPARE(static_cast<int>(back.attempts.size()), 2);
    QCOMPARE(textOf(back.attempts[0].date), QString("2026-01-01"));
    QCOMPARE(back.attempts[0].minutes, 35);
    QCOMPARE(textOf(back.attempts[0].outcome), QString("rejected"));
    QCOMPARE(textOf(back.attempts[0].note), QString("边界没想清楚"));
    QCOMPARE(back.attempts[1].minutes, 20);
    QCOMPARE(textOf(back.attempts[1].outcome), QString("ac"));
}

void JsonTaskStoreTest::migratesVersion5IfNoAttempts() {
    TempDir directory;
    QVERIFY(directory.isValid());
    const auto path = jsonPath(directory);
    QVERIFY(writeFile(directory.filePath("tasks.json"),
                      R"({"version":5,"tasks":[{"title":"v5 老文件","difficulty":"普及","difficultyRank":1200}]})"));

    const JsonLoadResult loaded = loadTasksJson(path);
    QVERIFY(loaded.ok);
    QVERIFY(loaded.migrated);
    QVERIFY(loaded.tasks[0].attempts.empty());
    auto snapshot = path;
    snapshot += ".pre-v6";
    QVERIFY(std::filesystem::exists(snapshot));
}

int main(int argc, char** argv) {
    JsonTaskStoreTest instance;
    return QTest::qExec(&instance, argc, argv);
}

#include "test_json_task_store.moc"
