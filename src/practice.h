#pragma once

#include <QDate>

#include <string>
#include <vector>

#include "task_store.h"

// 练习记录的结果取值，与 JSON 里的 outcome 字段一致。
namespace attempt_outcome {
inline constexpr const char* kAccepted = "ac";
inline constexpr const char* kRejected = "rejected";
inline constexpr const char* kUnfinished = "unfinished";
inline constexpr const char* kReviewed = "reviewed";
} // namespace attempt_outcome

// "ac" -> "通过"；未知值原样返回。
std::string attemptOutcomeLabel(const std::string& outcome);

struct AttemptSummary {
    int count = 0;
    int acCount = 0;
    int totalMinutes = 0;
    std::string lastDate;   // 最近一次练习的日期
    std::string lastOutcome;
};

AttemptSummary summarizeAttempts(const std::vector<Attempt>& attempts);

// 规则式建议：根据练习记录给一句话，没有可说的就返回空串。
std::string practiceAdvice(const Task& task, const QDate& today);
