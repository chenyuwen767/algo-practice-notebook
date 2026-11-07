#include "difficulty.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace {

struct LuoguLevel {
    const char* label;
    int rank;
};

// 洛谷官方档位到 CF 刻度的近似换算。顺序很重要：长档位要排在短档位前面，
// 否则 “普及/提高-” 会先命中 “普及”。
const LuoguLevel kLuoguLevels[] = {
    {"NOI/NOI+/CTSC", 2600},
    {"省选/NOI-", 2100},
    {"提高+/省选-", 1700},
    {"普及+/提高", 1300},
    {"普及/提高-", 1100},
    {"入门", 800},
    {"普及-", 1000},
    {"普及+", 1400},
    {"普及", 1200},
    {"提高+", 1800},
    {"提高", 1600},
    {"省选", 2200},
    {"NOI", 2500},
};

// 把全角减号（U+FF0D）与数学减号（U+2212）统一成 ASCII '-'，其余字节原样保留。
std::string normalize(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char first = static_cast<unsigned char>(text[i]);
        if (i + 2 < text.size() && first == 0xEF &&
            static_cast<unsigned char>(text[i + 1]) == 0xBC &&
            static_cast<unsigned char>(text[i + 2]) == 0x8D) {
            out.push_back('-');
            i += 3;
            continue;
        }
        if (i + 2 < text.size() && first == 0xE2 &&
            static_cast<unsigned char>(text[i + 1]) == 0x88 &&
            static_cast<unsigned char>(text[i + 2]) == 0x92) {
            out.push_back('-');
            i += 3;
            continue;
        }
        out.push_back(text[i]);
        ++i;
    }
    return out;
}

std::string trimAscii(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool startsWithDigits(const std::string& text) {
    return !text.empty() && std::isdigit(static_cast<unsigned char>(text[0])) != 0;
}

} // namespace

int difficultyRankFromText(const std::string& text) {
    const std::string trimmed = trimAscii(text);
    if (trimmed.empty()) {
        return 0;
    }

    // Codeforces 风格：直接是分数（800 / 1200 / 2400 …）。
    if (startsWithDigits(trimmed)) {
        int value = 0;
        for (char ch : trimmed) {
            if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
                break;
            }
            value = value * 10 + (ch - '0');
            if (value > 100000) {
                break;
            }
        }
        if (value < 800) {
            return 800;
        }
        return std::min(value, 3500);
    }

    // 洛谷档位：先去掉空格与全角减号变体，再按“最长匹配”查表。
    std::string label = normalize(trimmed);
    label.erase(std::remove_if(label.begin(), label.end(),
                               [](char ch) {
                                   return ch == ' ' || ch == '\t';
                               }),
                label.end());
    label.erase(std::remove(label.begin(), label.end(), '\r'), label.end());

    for (const LuoguLevel& level : kLuoguLevels) {
        if (label.find(level.label) != std::string::npos) {
            return level.rank;
        }
    }

    // 既不是分数开头也不是已知档位时，退一步找文本里的三位/四位数，
    // 兼容 “CF 1800”“洛谷 P1803 提高+” 这类混写。
    for (std::size_t i = 0; i < label.size();) {
        if (std::isdigit(static_cast<unsigned char>(label[i])) == 0) {
            ++i;
            continue;
        }
        std::size_t end = i;
        int value = 0;
        while (end < label.size() && std::isdigit(static_cast<unsigned char>(label[end])) != 0) {
            value = value * 10 + (label[end] - '0');
            ++end;
        }
        const std::size_t digits = end - i;
        if (digits >= 3 && value >= 800 && value <= 3500) {
            return value;
        }
        i = end;
    }
    return 0;
}

std::string difficultyBucket(int rank) {
    if (rank <= 0) {
        return "未标注";
    }
    if (rank < 1000) {
        return "入门";
    }
    if (rank < 1100) {
        return "普及-";
    }
    if (rank < 1300) {
        return "普及";
    }
    if (rank < 1500) {
        return "普及+";
    }
    if (rank < 1700) {
        return "提高";
    }
    if (rank < 1900) {
        return "提高+";
    }
    if (rank < 2400) {
        return "省选";
    }
    return "省选·NOI";
}

std::string difficultyDisplayText(const std::string& raw, int rank) {
    const std::string trimmed = trimAscii(raw);
    if (trimmed.empty()) {
        return "未标注";
    }
    if (rank <= 0) {
        return trimmed;
    }
    // 纯分数没必要再补一遍换算结果，否则显示成 “800（≈800）”。
    if (startsWithDigits(trimmed) && trimmed.find_first_not_of("0123456789") == std::string::npos) {
        return trimmed;
    }
    return trimmed + "（≈" + std::to_string(rank) + "）";
}
