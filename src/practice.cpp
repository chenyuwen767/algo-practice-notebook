#include "practice.h"

#include <QString>

namespace {

QDate parseDate(const std::string& text) {
    return QDate::fromString(QString::fromUtf8(text.c_str()), Qt::ISODate);
}

} // namespace

std::string attemptOutcomeLabel(const std::string& outcome) {
    if (outcome == attempt_outcome::kAccepted) {
        return "通过";
    }
    if (outcome == attempt_outcome::kRejected) {
        return "未通过";
    }
    if (outcome == attempt_outcome::kUnfinished) {
        return "没做完";
    }
    if (outcome == attempt_outcome::kReviewed) {
        return "看题解";
    }
    return outcome;
}

AttemptSummary summarizeAttempts(const std::vector<Attempt>& attempts) {
    AttemptSummary summary;
    summary.count = static_cast<int>(attempts.size());
    for (const Attempt& attempt : attempts) {
        summary.totalMinutes += attempt.minutes;
        if (attempt.outcome == attempt_outcome::kAccepted) {
            ++summary.acCount;
        }
        if (!attempt.date.empty()) {
            summary.lastDate = attempt.date;
        }
        summary.lastOutcome = attempt.outcome;
    }
    return summary;
}

std::string practiceAdvice(const Task& task, const QDate& today) {
    const AttemptSummary summary = summarizeAttempts(task.attempts);
    if (summary.count == 0) {
        if (task.completed) {
            return "已完成但没有练习记录：下次记一下耗时，统计才有依据。";
        }
        return "还没有练习记录：先做满 30 分钟，再在这里记一次。";
    }

    const Attempt& last = task.attempts.back();
    if (last.outcome == attempt_outcome::kAccepted && !task.completed) {
        return "最近一次已经通过，记得把状态标成已完成。";
    }
    if (last.outcome == attempt_outcome::kUnfinished || last.outcome == attempt_outcome::kRejected) {
        return "上次没通过，建议先看题解理清思路，隔一天再重做一遍。";
    }
    if (last.outcome == attempt_outcome::kReviewed) {
        return "看过题解后隔一天重做，别当天抄完就算了。";
    }

    if (summary.acCount == 1 && !summary.lastDate.empty()) {
        const QDate lastAccepted = parseDate(summary.lastDate);
        if (lastAccepted.isValid()) {
            const int days = lastAccepted.daysTo(today);
            if (days >= 14) {
                return "距上次通过已 " + std::to_string(days) + " 天，安排一次重做巩固。";
            }
        }
    }

    if (summary.acCount >= 1 && summary.totalMinutes > 0) {
        const int average = summary.totalMinutes / summary.count;
        if (average >= 90) {
            return "平均每次 " + std::to_string(average) + " 分钟，偏长：下次先把题目拆成两个小目标。";
        }
    }

    if (summary.acCount == 0 && summary.count >= 3) {
        return "这题已经记了 " + std::to_string(summary.count) +
               " 次还没通过，先降一档难度刷同类题。";
    }
    return {};
}
