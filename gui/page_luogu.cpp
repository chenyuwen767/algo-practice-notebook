#include "main_window.h"

using namespace taskui;

QWidget* MainWindow::createLuoguPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    addPageHeading(layout, "洛谷账号信息", "输入用户名或 UID，查询公开账号资料和解题统计。");

    auto* searchCard = createCard(page);
    auto* searchLayout = new QHBoxLayout(searchCard);
    luoguHandleInput_ = new QLineEdit(searchCard);
    luoguHandleInput_->setPlaceholderText("输入洛谷用户名或 UID");
    luoguButton_ = new QPushButton("查询账号", searchCard);
    luoguRefreshButton_ = new QPushButton("刷新", searchCard);
    luoguRefreshButton_->setToolTip("忽略 5 分钟缓存，重新请求");
    searchLayout->addWidget(new QLabel("账号", searchCard));
    searchLayout->addWidget(luoguHandleInput_);
    searchLayout->addWidget(luoguButton_);
    searchLayout->addWidget(luoguRefreshButton_);
    layout->addWidget(searchCard);

    luoguSummary_ = new QLabel("输入公开的洛谷用户名或 UID 后查询资料。", page);
    luoguSummary_->setObjectName("summaryCard");
    luoguSummary_->setWordWrap(true);
    layout->addWidget(luoguSummary_);

    luoguDetails_ = new QLabel("可显示通过题数、提交题数、等级分和关注数据。", page);
    luoguDetails_->setObjectName("hint");
    luoguDetails_->setWordWrap(true);
    layout->addWidget(luoguDetails_);

    auto* problemCard = createCard(page);
    auto* problemLayout = new QVBoxLayout(problemCard);
    auto* problemTitle = new QLabel("题目关联", problemCard);
    problemTitle->setStyleSheet("font-weight: 700; color: #314d75;");
    auto* problemRow = new QHBoxLayout;
    luoguProblemInput_ = new QLineEdit(problemCard);
    luoguProblemInput_->setPlaceholderText("输入洛谷题号，例如 P1001");
    luoguProblemButton_ = new QPushButton("查询题目", problemCard);
    luoguProblemRefreshButton_ = new QPushButton("刷新", problemCard);
    luoguProblemRefreshButton_->setToolTip("忽略 5 分钟缓存，重新请求");
    problemRow->addWidget(luoguProblemInput_);
    problemRow->addWidget(luoguProblemButton_);
    problemRow->addWidget(luoguProblemRefreshButton_);
    luoguProblemInfo_ = new QLabel(
        "查询公开题目信息后，可关联到当前选中的任务，或直接新建任务。", problemCard);
    luoguProblemInfo_->setObjectName("hint");
    luoguProblemInfo_->setWordWrap(true);
    auto* problemActions = new QHBoxLayout;
    luoguLinkSelectedButton_ = new QPushButton("关联到选中任务", problemCard);
    luoguCreateTaskButton_ = new QPushButton("新建任务", problemCard);
    luoguLinkSelectedButton_->setEnabled(false);
    luoguCreateTaskButton_->setEnabled(false);
    problemActions->addWidget(luoguLinkSelectedButton_);
    problemActions->addWidget(luoguCreateTaskButton_);
    problemActions->addStretch();
    problemLayout->addWidget(problemTitle);
    problemLayout->addLayout(problemRow);
    problemLayout->addWidget(luoguProblemInfo_);
    problemLayout->addLayout(problemActions);
    layout->addWidget(problemCard);

    auto* limitationCard = createCard(page);
    auto* limitationLayout = new QVBoxLayout(limitationCard);
    auto* limitationTitle = new QLabel("提交记录说明", limitationCard);
    limitationTitle->setStyleSheet("font-weight: 700; color: #314d75;");
    auto* limitation = new QLabel(
        "洛谷当前可能要求登录后才能读取他人的提交明细。此页面只读取公开资料，不会请求、保存或传输账号密码。",
        limitationCard);
    limitation->setObjectName("hint");
    limitation->setWordWrap(true);
    limitationLayout->addWidget(limitationTitle);
    limitationLayout->addWidget(limitation);
    layout->addWidget(limitationCard);
    layout->addStretch();

    connect(luoguButton_, &QPushButton::clicked, this, [this] { loadLuoguAccount(); });
    connect(luoguRefreshButton_, &QPushButton::clicked, this,
            [this] { loadLuoguAccount(true); });
    connect(luoguHandleInput_, &QLineEdit::returnPressed, this, [this] { loadLuoguAccount(); });
    connect(luoguProblemButton_, &QPushButton::clicked, this, [this] { lookupLuoguProblem(); });
    connect(luoguProblemRefreshButton_, &QPushButton::clicked, this,
            [this] { lookupLuoguProblem(true); });
    connect(luoguProblemInput_, &QLineEdit::returnPressed, this,
            [this] { lookupLuoguProblem(); });
    connect(luoguProblemInput_, &QLineEdit::textChanged, this, [this] {
        luoguProblem_ = {};
        luoguLinkSelectedButton_->setEnabled(false);
        luoguCreateTaskButton_->setEnabled(false);
    });
    connect(luoguLinkSelectedButton_, &QPushButton::clicked, this,
            [this] { linkLuoguProblemToSelectedTask(); });
    connect(luoguCreateTaskButton_, &QPushButton::clicked, this,
            [this] { createTaskFromLuoguProblem(); });
    return page;
}

void MainWindow::loadLuoguAccount(bool forceRefresh) {
    if (luoguLoading_) {
        return;
    }

    const QString account = luoguHandleInput_->text().trimmed();
    if (account.isEmpty()) {
        QMessageBox::information(this, "需要账号", "请输入洛谷用户名或 UID。");
        return;
    }

    luoguLoading_ = true;
    luoguButton_->setEnabled(false);
    settings_.setValue("luogu/handle", account);
    luoguSummary_->setText("正在查询公开资料……");
    luoguDetails_->setText("正在读取账号统计……");

    bool isUid = false;
    const int uid = account.toInt(&isUid);
    if (isUid) {
        loadLuoguProfile(uid, forceRefresh);
        return;
    }

    QUrl url("https://www.luogu.com.cn/api/user/search");
    QUrlQuery query;
    query.addQueryItem("keyword", account);
    url.setQuery(query);

    ojClient_->get(OjPlatform::Luogu,
                   url,
                   forceRefresh,
                   [this, account, forceRefresh](const OjResponse& response) {
                       QJsonObject json;
                       QString error;
                       if (!readJsonObject(response, json, error)) {
                           showLuoguError(error);
                           return;
                       }

                       const QJsonArray users = json.value("users").toArray();
                       if (users.isEmpty()) {
                           showLuoguError("未找到匹配的公开账号。");
                           return;
                       }

                       QJsonObject selected = users.first().toObject();
                       for (const QJsonValue& value : users) {
                           const QJsonObject candidate = value.toObject();
                           if (candidate.value("name").toString().compare(
                                   account, Qt::CaseInsensitive) == 0) {
                               selected = candidate;
                               break;
                           }
                       }

                       loadLuoguProfile(selected.value("uid").toInt(), forceRefresh);
                   });
}

void MainWindow::loadLuoguProfile(int uid, bool forceRefresh) {
    const QUrl url("https://www.luogu.com.cn/api/user/info/" + QString::number(uid));
    ojClient_->get(OjPlatform::Luogu,
                   url,
                   forceRefresh,
                   [this](const OjResponse& response) {
                       QJsonObject json;
                       QString error;
                       if (!readJsonObject(response, json, error)) {
                           showLuoguError(error);
                           return;
                       }

                       const QJsonObject user = json.value("user").toObject();
                       if (user.isEmpty()) {
                           showLuoguError("该账号资料不可见或不存在。");
                           return;
                       }

                       const QString elo = user.value("eloValue").isDouble()
                           ? QString::number(user.value("eloValue").toDouble(), 'f', 0)
                           : "未公开";
                       luoguSummary_->setText(
                           QString("%1（UID：%2）\n通过 %3 题 · 提交 %4 题 · 等级分：%5")
                               .arg(user.value("name").toString())
                               .arg(user.value("uid").toInt())
                               .arg(optionalNumber(user.value("passedProblemCount")))
                               .arg(optionalNumber(user.value("submittedProblemCount")))
                               .arg(elo));
                       luoguDetails_->setText(
                           QString("CCF 等级：%1  ·  XCPC 等级：%2  ·  关注：%3  ·  粉丝：%4\n%5")
                               .arg(optionalNumber(user.value("ccfLevel")))
                               .arg(optionalNumber(user.value("xcpcLevel")))
                               .arg(optionalNumber(user.value("followingCount")))
                               .arg(optionalNumber(user.value("followerCount")))
                               .arg(user.value("slogan").toString("这个人很懒，什么都没写")));
                       luoguLoading_ = false;
                       luoguButton_->setEnabled(true);
                   });
}

void MainWindow::lookupLuoguProblem(bool forceRefresh) {
    if (luoguProblemLoading_) {
        return;
    }

    const QString pid = normalizeLuoguProblemId(luoguProblemInput_->text());
    if (pid.isEmpty()) {
        luoguProblem_ = {};
        luoguLinkSelectedButton_->setEnabled(false);
        luoguCreateTaskButton_->setEnabled(false);
        luoguProblemInfo_->setText("请输入有效的洛谷题号，例如 P1001。");
        return;
    }

    luoguProblemLoading_ = true;
    luoguProblem_ = {};
    luoguProblemButton_->setEnabled(false);
    luoguLinkSelectedButton_->setEnabled(false);
    luoguCreateTaskButton_->setEnabled(false);
    luoguProblemInfo_->setText("正在查询公开题目信息……");

    ojClient_->get(OjPlatform::Luogu,
                   luoguProblemUrl(pid),
                   forceRefresh,
                   [this](const OjResponse& response) {
                       QString error;
                       if (response.httpStatus == 404) {
                           error = "题号不存在或题目不可见。";
                       } else if (response.httpStatus != 0 &&
                                  response.httpStatus != 200) {
                           if (!readHttpError(response.httpStatus, error)) {
                               error = QString("洛谷返回 HTTP %1。")
                                           .arg(response.httpStatus);
                           }
                       } else if (!response.ok) {
                           error = "网络请求失败：" + response.error;
                       }

                       luoguProblemLoading_ = false;
                       luoguProblemButton_->setEnabled(true);
                       if (!error.isEmpty()) {
                           luoguProblemInfo_->setText("查询失败：" + error);
                           return;
                       }

                       LuoguProblem problem;
                       QString parseError;
                       if (!parseLuoguProblemHtml(QString::fromUtf8(response.body),
                                                  problem,
                                                  parseError)) {
                           luoguProblemInfo_->setText("查询失败：" + parseError);
                           return;
                       }

                       luoguProblem_ = problem;
                       luoguProblemInfo_->setText(
                           QString("%1 %2\n难度：%3 · 通过 %4 / 提交 %5")
                               .arg(problem.pid,
                                    problem.name,
                                    luoguDifficultyLabel(problem.difficulty))
                               .arg(problem.totalAccepted)
                               .arg(problem.totalSubmit));
                       luoguLinkSelectedButton_->setEnabled(true);
                       luoguCreateTaskButton_->setEnabled(true);
                   });
}

void MainWindow::linkLuoguProblemToSelectedTask() {
    if (!luoguProblem_.valid) {
        QMessageBox::information(this, "还没有题目", "请先查询洛谷题目。");
        return;
    }
    const int index = selectedTaskIndex();
    if (index < 0) {
        QMessageBox::information(this, "请选择题目", "请先在任务表格中选择一道题目。");
        return;
    }

    rememberState();
    Task& task = tasks_[static_cast<std::size_t>(index)];
    task.platform = "luogu";
    task.problemId = toTaskText(luoguProblem_.pid);
    const QString taskTitle = taskText(task.title);
    saveAndRefresh();
    luoguProblemInfo_->setText(
        QString("已把 %1 %2 关联到任务“%3”。")
            .arg(luoguProblem_.pid, luoguProblem_.name, taskTitle));
}

void MainWindow::createTaskFromLuoguProblem() {
    if (!luoguProblem_.valid) {
        QMessageBox::information(this, "还没有题目", "请先查询洛谷题目。");
        return;
    }

    QString title = luoguProblem_.name.isEmpty() ? luoguProblem_.pid : luoguProblem_.name;
    QString category = "洛谷";
    QString difficulty = luoguDifficultyLabel(luoguProblem_.difficulty);
    QString platform = "luogu";
    QString problemId = luoguProblem_.pid;
    QString tags;
    QString notes;
    if (!promptTaskFields(this, "从洛谷题目新建任务", title, category, difficulty, platform,
                          problemId, tags, notes)) {
        return;
    }
    if (taskTitleExists(title)) {
        const auto answer = QMessageBox::question(
            this,
            "题目已存在",
            QString("“%1”已经在任务列表中，仍要再添加一条吗？").arg(title),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    rememberState();
    Task task;
    task.title = toTaskText(title);
    task.category = toTaskText(category);
    task.difficulty = toTaskText(difficulty);
    task.platform = toTaskText(platform);
    task.problemId = toTaskText(problemId);
    task.tags = toTaskTags(tags);
    task.notes = toTaskText(notes);
    task.id = newTaskId();
    task.createdAt = nowIso();
    tasks_.push_back(task);
    saveAndRefresh();
    QMessageBox::information(this, "已新建任务", QString("已新建：%1").arg(title));
}

void MainWindow::showLuoguError(const QString& error) {
    luoguLoading_ = false;
    luoguSummary_->setText("查询失败：" + error);
    luoguDetails_->setText("请检查用户名或 UID，并确认账号资料公开可见。");
    luoguButton_->setEnabled(true);
}
