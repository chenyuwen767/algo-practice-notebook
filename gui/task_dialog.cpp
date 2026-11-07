#include "task_dialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "codeforces_problem.h"

namespace taskui {

bool promptTaskFields(QWidget* parent,
                      const QString& dialogTitle,
                      QString& title,
                      QString& category,
                      QString& difficulty,
                      QString& platform,
                      QString& problemId,
                      QString& tags,
                      QString& notes) {
    QDialog dialog(parent);
    dialog.setWindowTitle(dialogTitle);
    dialog.resize(480, 400);

    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;
    auto* titleEdit = new QLineEdit(title, &dialog);
    auto* categoryEdit = new QLineEdit(category, &dialog);
    auto* difficultyEdit = new QLineEdit(difficulty, &dialog);
    auto* platformEdit = new QComboBox(&dialog);
    platformEdit->addItem("（无）", QString());
    platformEdit->addItem("Codeforces", QString("codeforces"));
    platformEdit->addItem("洛谷", QString("luogu"));
    const int platformIndex = platformEdit->findData(platform);
    platformEdit->setCurrentIndex(platformIndex >= 0 ? platformIndex : 0);
    auto* problemIdEdit = new QLineEdit(problemId, &dialog);
    problemIdEdit->setPlaceholderText("例如 1900A 或 P1001");
    auto* tagsEdit = new QLineEdit(tags, &dialog);
    auto* notesEdit = new QPlainTextEdit(notes, &dialog);
    tagsEdit->setPlaceholderText("多个标签用逗号分隔，例如：trie, xor");
    notesEdit->setPlaceholderText("可记录思路、易错点、重做提醒等");
    notesEdit->setFixedHeight(80);
    form->addRow("题目名称：", titleEdit);
    form->addRow("分类：", categoryEdit);
    form->addRow("难度：", difficultyEdit);
    form->addRow("平台：", platformEdit);
    form->addRow("题号：", problemIdEdit);
    form->addRow("标签：", tagsEdit);
    form->addRow("备注：", notesEdit);
    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("确定");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    titleEdit->setFocus();

    while (dialog.exec() == QDialog::Accepted) {
        title = titleEdit->text().trimmed();
        category = categoryEdit->text().trimmed();
        difficulty = difficultyEdit->text().trimmed();
        platform = platformEdit->currentData().toString();
        problemId = problemIdEdit->text().trimmed();
        if (platform == "codeforces" && !problemId.isEmpty()) {
            const QString normalized = normalizeCodeforcesProblemId(problemId);
            if (!normalized.isEmpty()) {
                problemId = normalized;
            }
        }
        tags = tagsEdit->text().trimmed();
        notes = notesEdit->toPlainText().trimmed();
        if (!title.isEmpty() && !category.isEmpty() && !difficulty.isEmpty()) {
            return true;
        }
        QMessageBox::warning(&dialog, "无法保存", "题目名称、分类和难度都不能为空。");
    }
    return false;
}

} // namespace taskui
