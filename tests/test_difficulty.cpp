#include <QtTest>

#include <string>

#include "difficulty.h"

namespace {

std::string utf8(const char* text) {
    return std::string(text);
}

} // namespace

class DifficultyTest : public QObject {
    Q_OBJECT

private slots:
    void parsesCodeforcesRatings();
    void parsesLuoguLevels();
    void handlesMessyInput();
    void bucketsByRank();
    void formatsDisplayText();
};

void DifficultyTest::parsesCodeforcesRatings() {
    QCOMPARE(difficultyRankFromText("800"), 800);
    QCOMPARE(difficultyRankFromText("1600"), 1600);
    QCOMPARE(difficultyRankFromText(" 2400 "), 2400);
    // 低于 800 视为 800，超过 3500 截到 3500，避免脏数据把分布拉坏。
    QCOMPARE(difficultyRankFromText("300"), 800);
    QCOMPARE(difficultyRankFromText("9999"), 3500);
}

void DifficultyTest::parsesLuoguLevels() {
    QCOMPARE(difficultyRankFromText(utf8("入门")), 800);
    QCOMPARE(difficultyRankFromText(utf8("普及-")), 1000);
    QCOMPARE(difficultyRankFromText(utf8("普及")), 1200);
    QCOMPARE(difficultyRankFromText(utf8("普及+")), 1400);
    QCOMPARE(difficultyRankFromText(utf8("提高")), 1600);
    QCOMPARE(difficultyRankFromText(utf8("提高+")), 1800);
    QCOMPARE(difficultyRankFromText(utf8("省选")), 2200);
    QCOMPARE(difficultyRankFromText(utf8("NOI")), 2500);
    // 复合档位按最长匹配，不能先命中 “普及” 或 “提高”。
    QCOMPARE(difficultyRankFromText(utf8("普及/提高-")), 1100);
    QCOMPARE(difficultyRankFromText(utf8("普及+/提高")), 1300);
    QCOMPARE(difficultyRankFromText(utf8("提高+/省选-")), 1700);
    QCOMPARE(difficultyRankFromText(utf8("省选/NOI-")), 2100);
    QCOMPARE(difficultyRankFromText(utf8("NOI/NOI+/CTSC")), 2600);
}

void DifficultyTest::handlesMessyInput() {
    QCOMPARE(difficultyRankFromText(""), 0);
    QCOMPARE(difficultyRankFromText("   "), 0);
    QCOMPARE(difficultyRankFromText(utf8("尚未评定")), 0);
    // 全角减号要能被识别。
    QCOMPARE(difficultyRankFromText(utf8("普及\xEF\xBC\x8D")), 1000);
    // 混写时退而求其次，从文本里找分数。
    QCOMPARE(difficultyRankFromText("CF 1800"), 1800);
    QCOMPARE(difficultyRankFromText(utf8("洛谷 P17633 提高+")), 1800);
}

void DifficultyTest::bucketsByRank() {
    QCOMPARE(QString::fromStdString(difficultyBucket(0)), QString("未标注"));
    QCOMPARE(QString::fromStdString(difficultyBucket(800)), QString("入门"));
    QCOMPARE(QString::fromStdString(difficultyBucket(1000)), QString("普及-"));
    QCOMPARE(QString::fromStdString(difficultyBucket(1200)), QString("普及"));
    QCOMPARE(QString::fromStdString(difficultyBucket(1400)), QString("普及+"));
    QCOMPARE(QString::fromStdString(difficultyBucket(1600)), QString("提高"));
    QCOMPARE(QString::fromStdString(difficultyBucket(1800)), QString("提高+"));
    QCOMPARE(QString::fromStdString(difficultyBucket(2100)), QString("省选"));
    QCOMPARE(QString::fromStdString(difficultyBucket(2600)), QString("省选·NOI"));
    // 洛谷档位换算回来要落回同名档位。
    QCOMPARE(QString::fromStdString(difficultyBucket(difficultyRankFromText(utf8("普及-")))), QString("普及-"));
    QCOMPARE(QString::fromStdString(difficultyBucket(difficultyRankFromText(utf8("提高+")))), QString("提高+"));
}

void DifficultyTest::formatsDisplayText() {
    QCOMPARE(QString::fromStdString(difficultyDisplayText(utf8("普及-"), 1000)),
             QString("普及-（≈1000）"));
    QCOMPARE(QString::fromStdString(difficultyDisplayText("", 0)), QString("未标注"));
    QCOMPARE(QString::fromStdString(difficultyDisplayText(utf8("自定义"), 0)),
             QString("自定义"));
    QCOMPARE(QString::fromStdString(difficultyDisplayText("  ", 0)), QString("未标注"));
}

int main(int argc, char** argv) {
    DifficultyTest instance;
    return QTest::qExec(&instance, argc, argv);
}

#include "test_difficulty.moc"
