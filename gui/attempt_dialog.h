#pragma once

#include <QString>

class QWidget;

// 记录一次练习：日期默认今天，结果用四选一，备注可空。
bool promptAttempt(QWidget* parent,
                   const QString& title,
                   QString& date,
                   int& minutes,
                   QString& outcome,
                   QString& note);
