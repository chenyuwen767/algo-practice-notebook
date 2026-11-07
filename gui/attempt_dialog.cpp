#include "attempt_dialog.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

#include "practice.h"

bool promptAttempt(QWidget* parent,
                   const QString& title,
                   QString& date,
                   int& minutes,
                   QString& outcome,
                   QString& note) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(420);

    auto* form = new QFormLayout(&dialog);

    auto* dateEdit = new QDateEdit(QDate::currentDate(), &dialog);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    form->addRow("练习日期：", dateEdit);

    auto* minutesSpin = new QSpinBox(&dialog);
    minutesSpin->setRange(0, 600);
    minutesSpin->setSuffix(" 分钟");
    minutesSpin->setSpecialValueText("没记");
    minutesSpin->setValue(30);
    form->addRow("本次耗时：", minutesSpin);

    auto* outcomeBox = new QComboBox(&dialog);
    outcomeBox->addItem("通过", QString::fromUtf8(attempt_outcome::kAccepted));
    outcomeBox->addItem("未通过", QString::fromUtf8(attempt_outcome::kRejected));
    outcomeBox->addItem("没做完", QString::fromUtf8(attempt_outcome::kUnfinished));
    outcomeBox->addItem("看题解", QString::fromUtf8(attempt_outcome::kReviewed));
    form->addRow("结果：", outcomeBox);

    auto* noteEdit = new QLineEdit(&dialog);
    noteEdit->setPlaceholderText("卡在哪一步、下次想先做什么（可空）");
    form->addRow("备注：", noteEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    date = dateEdit->date().toString(Qt::ISODate);
    minutes = minutesSpin->value();
    outcome = outcomeBox->currentData().toString();
    note = noteEdit->text().trimmed();
    return true;
}
