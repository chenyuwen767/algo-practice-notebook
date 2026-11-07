#pragma once

#include <QString>

class QWidget;

namespace taskui {

// 新增/编辑题目共用的单表单对话框。用户确认且必填项齐全时返回 true。
bool promptTaskFields(QWidget* parent,
                      const QString& dialogTitle,
                      QString& title,
                      QString& category,
                      QString& difficulty,
                      QString& platform,
                      QString& problemId,
                      QString& tags,
                      QString& notes);

} // namespace taskui
