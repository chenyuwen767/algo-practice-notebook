#include "main_window.h"

using namespace taskui;

QWidget* MainWindow::createStatsPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    addPageHeading(layout, "学习统计", "查看完成率、打卡连续天数、分布和最近 8 周趋势。");

    statsSummary_ = new QLabel("正在统计……", page);
    statsSummary_->setObjectName("summaryCard");
    statsSummary_->setWordWrap(true);
    layout->addWidget(statsSummary_);

    auto* goalCard = createCard(page);
    auto* goalLayout = new QHBoxLayout(goalCard);
    statsDailyLabel_ = new QLabel("今日完成 0 题", goalCard);
    statsGoalSpin_ = new QSpinBox(goalCard);
    statsGoalSpin_->setRange(1, 50);
    statsGoalSpin_->setPrefix("目标 ");
    statsGoalSpin_->setSuffix(" 题/天");
    statsGoalSpin_->setValue(settings_.value("stats/dailyGoal", 3).toInt());
    statsDailyBar_ = new QProgressBar(goalCard);
    statsDailyBar_->setRange(0, 100);
    statsDailyBar_->setValue(0);
    statsDailyBar_->setTextVisible(true);
    statsDailyBar_->setFormat("0%");
    goalLayout->addWidget(statsDailyLabel_);
    goalLayout->addWidget(statsGoalSpin_);
    goalLayout->addWidget(statsDailyBar_, 1);
    layout->addWidget(goalCard);

    auto* distributionCard = createCard(page);
    auto* distributionLayout = new QVBoxLayout(distributionCard);
    auto* distributionTitle = new QLabel("完成分布", distributionCard);
    distributionTitle->setStyleSheet("font-weight: 700; color: #314d75;");
    statsDistribution_ = new QLabel("正在统计……", distributionCard);
    statsDistribution_->setObjectName("hint");
    statsDistribution_->setWordWrap(true);
    distributionLayout->addWidget(distributionTitle);
    distributionLayout->addWidget(statsDistribution_);
    layout->addWidget(distributionCard);

    auto* trendCard = createCard(page);
    auto* trendLayout = new QVBoxLayout(trendCard);
    auto* trendTitle = new QLabel("最近 8 周完成趋势", trendCard);
    trendTitle->setStyleSheet("font-weight: 700; color: #314d75;");
    statsWeeklyChart_ = new WeeklyChart(trendCard);
    trendLayout->addWidget(trendTitle);
    trendLayout->addWidget(statsWeeklyChart_);
    layout->addWidget(trendCard);

    statsUntimed_ = new QLabel(page);
    statsUntimed_->setObjectName("hint");
    statsUntimed_->setWordWrap(true);
    statsUntimed_->hide();
    layout->addWidget(statsUntimed_);
    layout->addStretch();

    connect(statsGoalSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        settings_.setValue("stats/dailyGoal", value);
        updateStatsPage();
    });
    return page;
}

void MainWindow::updateStatsPage() {
    if (statsSummary_ == nullptr) {
        return;
    }

    const int goal = statsGoalSpin_ ? statsGoalSpin_->value() : 3;
    const StudyStats stats = computeStudyStats(tasks_, QDate::currentDate());
    const int rate = stats.total == 0 ? 0 : stats.completed * 100 / stats.total;

    statsSummary_->setText(
        QString("总题数 %1 · 已完成 %2（%3%）\n"
                "今日 %4 · 本周 %5 · 本月 %6 · 连续打卡 %7 天")
            .arg(stats.total)
            .arg(stats.completed)
            .arg(rate)
            .arg(stats.today)
            .arg(stats.week)
            .arg(stats.month)
            .arg(stats.streak));

    statsDailyLabel_->setText(QString("今日完成 %1 题").arg(stats.today));
    statsDailyBar_->setValue(qBound(0, stats.today * 100 / std::max(1, goal), 100));
    statsDailyBar_->setFormat(QString("%1/%2").arg(stats.today).arg(goal));

    const auto formatDistribution = [](const QMap<QString, int>& totals,
                                       const QMap<QString, int>& completed,
                                       bool platform) {
        QStringList parts;
        for (auto it = totals.cbegin(); it != totals.cend(); ++it) {
            const QString label = platform ? platformLabel(it.key()) : it.key();
            parts << QString("%1 %2/%3")
                         .arg(label)
                         .arg(completed.value(it.key()))
                         .arg(it.value());
        }
        return parts.isEmpty() ? QString("暂无数据") : parts.join(" · ");
    };

    statsDistribution_->setText(
        QString("按平台：%1\n按分类：%2\n按难度：%3")
            .arg(formatDistribution(stats.totalByPlatform, stats.completedByPlatform, true),
                 formatDistribution(stats.totalByCategory, stats.completedByCategory, false),
                 formatDistribution(stats.totalByDifficulty, stats.completedByDifficulty, false)));

    if (stats.untimedCompleted > 0) {
        statsUntimed_->setText(
            QString("另有 %1 道已完成任务没有完成时间，未计入今日、本周和连续打卡统计。")
                .arg(stats.untimedCompleted));
        statsUntimed_->show();
    } else {
        statsUntimed_->hide();
    }

    if (statsWeeklyChart_) {
        statsWeeklyChart_->setValues(stats.weekly);
    }
}
