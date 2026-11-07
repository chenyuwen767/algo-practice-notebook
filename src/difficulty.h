#pragma once

#include <string>

// 把各种写法的难度（Codeforces 分数、洛谷档位）归一到一个可比较的 rank。
// 0 表示未标注；数值越大越难，刻度与 Codeforces rating 对齐（800 起）。
int difficultyRankFromText(const std::string& text);

// rank 归到 6 档（未标注 / 入门 / 普及 / 普及+ / 提高 / 提高+ / 省选·NOI）。
std::string difficultyBucket(int rank);

// 表格里的显示文本：能识别时写成 “洛谷 普及-（≈1000）”，否则保持原样。
std::string difficultyDisplayText(const std::string& raw, int rank);
