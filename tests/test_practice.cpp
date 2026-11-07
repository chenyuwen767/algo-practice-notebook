#include <QtTest>

#include <string>
#include <vector>

#include "practice.h"

namespace {

Attempt attempt(const char* date, int minutes, const char* outcome) {
    Attempt item;
    item.date = date;
    item.minutes = minutes;
    item.outcome = outcome;
    return item;
}

Task taskWith(const std::vector<Attempt>& attempts) {
    Task task;
    task.attempts = attempts;
    return task;
}

} // namespace

class PracticeTest : public QObject {
    Q_OBJECT

private slots:
    void labelsOutcomes();
    void summarizesAttempts();
    void advisesOnEmptyHistory();
    void advisesWhenAcceptedButNotCompleted();
    void advisesAfterFailure();
    void advisesToRedoAfterReadingSolution();
    void advisesReviewAfterLongGap();
    void advisesWhenAttemptsAreSlow();
    void advisesToLowerDifficulty();
    void staysQuietWhenNothingToSay();
};

void PracticeTest::labelsOutcomes() {
    QCOMPARE(attemptOutcomeLabel(attempt_outcome::kAccepted), std::string("通过"));
    QCOMPARE(attemptOutcomeLabel(attempt_outcome::kRejected), std::string("未通过"));
    QCOMPARE(attemptOutcomeLabel(attempt_outcome::kUnfinished), std::string("没做完"));
    QCOMPARE(attemptOutcomeLabel(attempt_outcome::kReviewed), std::string("看题解"));
    // 未知取值原样返回，方便看 JSON 里到底写了什么。
    QCOMPARE(attemptOutcomeLabel("weird"), std::string("weird"));
}

void PracticeTest::summarizesAttempts() {
    const AttemptSummary summary = summarizeAttempts({
        attempt("2026-01-01", 40, attempt_outcome::kRejected),
        attempt("2026-01-03", 50, attempt_outcome::kAccepted),
        attempt("", 10, attempt_outcome::kReviewed),
    });
    QCOMPARE(summary.count, 3);
    QCOMPARE(summary.acCount, 1);
    QCOMPARE(summary.totalMinutes, 100);
    // 空日期不覆盖最近日期。
    QCOMPARE(summary.lastDate, std::string("2026-01-03"));
    QCOMPARE(summary.lastOutcome, std::string(attempt_outcome::kReviewed));
}

void PracticeTest::advisesOnEmptyHistory() {
    Task unfinished;
    QVERIFY(!practiceAdvice(unfinished, QDate(2026, 1, 3)).empty());

    Task done;
    done.completed = true;
    const std::string advice = practiceAdvice(done, QDate(2026, 1, 3));
    QVERIFY(!advice.empty());
    QVERIFY(advice != practiceAdvice(unfinished, QDate(2026, 1, 3)));
}

void PracticeTest::advisesWhenAcceptedButNotCompleted() {
    Task task = taskWith({attempt("2026-01-02", 30, attempt_outcome::kAccepted)});
    QVERIFY(!practiceAdvice(task, QDate(2026, 1, 3)).empty());
    QVERIFY(practiceAdvice(task, QDate(2026, 1, 3)).find("已完成") != std::string::npos);
}

void PracticeTest::advisesAfterFailure() {
    Task task = taskWith({
        attempt("2026-01-01", 30, attempt_outcome::kAccepted),
        attempt("2026-01-02", 25, attempt_outcome::kRejected),
    });
    task.completed = true;
    QVERIFY(!practiceAdvice(task, QDate(2026, 1, 3)).empty());
}

void PracticeTest::advisesToRedoAfterReadingSolution() {
    Task task = taskWith({attempt("2026-01-02", 20, attempt_outcome::kReviewed)});
    task.completed = true;
    QVERIFY(!practiceAdvice(task, QDate(2026, 1, 3)).empty());
}

void PracticeTest::advisesReviewAfterLongGap() {
    Task task = taskWith({attempt("2026-01-01", 20, attempt_outcome::kAccepted)});
    task.completed = true;
    QVERIFY(practiceAdvice(task, QDate(2026, 1, 10)).empty());
    const std::string advice = practiceAdvice(task, QDate(2026, 1, 20));
    QVERIFY(advice.find("19") != std::string::npos);
    QVERIFY(advice.find("重做") != std::string::npos);
}

void PracticeTest::advisesWhenAttemptsAreSlow() {
    Task task = taskWith({
        attempt("2026-01-01", 120, attempt_outcome::kAccepted),
        attempt("2026-01-02", 100, attempt_outcome::kAccepted),
    });
    task.completed = true;
    QVERIFY(!practiceAdvice(task, QDate(2026, 1, 3)).empty());
}

void PracticeTest::advisesToLowerDifficulty() {
    Task task = taskWith({
        attempt("2026-01-01", 30, attempt_outcome::kRejected),
        attempt("2026-01-02", 30, attempt_outcome::kRejected),
        attempt("2026-01-03", 30, attempt_outcome::kRejected),
    });
    QVERIFY(!practiceAdvice(task, QDate(2026, 1, 4)).empty());
}

void PracticeTest::staysQuietWhenNothingToSay() {
    Task task = taskWith({attempt("2026-01-02", 30, attempt_outcome::kAccepted)});
    task.completed = true;
    QVERIFY(practiceAdvice(task, QDate(2026, 1, 3)).empty());
}

QTEST_MAIN(PracticeTest)

#include "test_practice.moc"
