#include "main_window.h"

#include "difficulty.h"

using namespace taskui;

QWidget* MainWindow::createCodeforcesPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    addPageHeading(layout, "Codeforces 进度", "查询公开账号资料和最近提交记录。");

    auto* searchCard = createCard(page);
    auto* searchLayout = new QHBoxLayout(searchCard);
    codeforcesHandleInput_ = new QLineEdit(searchCard);
    codeforcesHandleInput_->setPlaceholderText("输入 Codeforces handle");
    codeforcesButton_ = new QPushButton("查询账号", searchCard);
    codeforcesRefreshButton_ = new QPushButton("刷新", searchCard);
    codeforcesRefreshButton_->setToolTip("忽略 5 分钟缓存，重新请求");
    searchLayout->addWidget(new QLabel("账号", searchCard));
    searchLayout->addWidget(codeforcesHandleInput_);
    searchLayout->addWidget(codeforcesButton_);
    searchLayout->addWidget(codeforcesRefreshButton_);
    layout->addWidget(searchCard);

    codeforcesSummary_ = new QLabel("输入公开的 Codeforces handle 后查询个人资料和提交记录。", page);
    codeforcesSummary_->setObjectName("summaryCard");
    codeforcesSummary_->setWordWrap(true);
    layout->addWidget(codeforcesSummary_);

    auto* note = new QLabel("通过题数根据最近 1000 次提交统计；完整历史可能超出此范围。", page);
    note->setObjectName("hint");
    note->setWordWrap(true);
    layout->addWidget(note);

    auto* taskActions = new QHBoxLayout;
    codeforcesVerifyButton_ = new QPushButton("核验关联任务", page);
    codeforcesImportButton_ = new QPushButton("导入最近 AC", page);
    codeforcesVerifyButton_->setEnabled(false);
    codeforcesImportButton_->setEnabled(false);
    codeforcesVerifyButton_->setToolTip("把已 AC 且已关联 Codeforces 题号的任务标记为完成");
    codeforcesImportButton_->setToolTip("把最近通过但尚未记录在任务列表中的题目导入");
    taskActions->addWidget(codeforcesVerifyButton_);
    taskActions->addWidget(codeforcesImportButton_);
    taskActions->addStretch();
    layout->addLayout(taskActions);

    codeforcesSubmissionsTable_ = new QTableWidget(page);
    codeforcesSubmissionsTable_->setColumnCount(4);
    codeforcesSubmissionsTable_->setHorizontalHeaderLabels({"提交时间", "题目", "结果", "语言"});
    codeforcesSubmissionsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    codeforcesSubmissionsTable_->setAlternatingRowColors(true);
    codeforcesSubmissionsTable_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(codeforcesSubmissionsTable_);

    connect(codeforcesButton_, &QPushButton::clicked, this, [this] { loadCodeforcesAccount(); });
    connect(codeforcesRefreshButton_, &QPushButton::clicked, this,
            [this] { loadCodeforcesAccount(true); });
    connect(codeforcesHandleInput_, &QLineEdit::returnPressed, this,
            [this] { loadCodeforcesAccount(); });
    connect(codeforcesVerifyButton_, &QPushButton::clicked, this,
            [this] { verifyCodeforcesTasks(); });
    connect(codeforcesImportButton_, &QPushButton::clicked, this,
            [this] { importCodeforcesSolved(); });
    return page;
}

QUrl MainWindow::codeforcesApiUrl(const QString& method, const QString& parameter, const QString& value) const {
    QUrl url("https://codeforces.com/api/" + method);
    QUrlQuery query;
    query.addQueryItem(parameter, value);
    if (method == "user.status") {
        query.addQueryItem("from", "1");
        query.addQueryItem("count", "1000");
    }
    url.setQuery(query);
    return url;
}

bool MainWindow::readHttpError(int statusCode, QString& error) const {
    if (statusCode == 429) {
        error = "请求过于频繁，请稍后重试。";
        return true;
    }
    if (statusCode == 404) {
        error = "账号不存在或资料不可见。";
        return true;
    }
    if (statusCode == 400) {
        error = "请求参数有误，请检查账号名。";
        return true;
    }
    if (statusCode >= 500) {
        error = "平台暂时不可用，请稍后重试。";
        return true;
    }
    return false;
}

bool MainWindow::readCodeforcesResult(const OjResponse& response,
                          QJsonArray& result,
                          QString& error) const {
    if (readHttpError(response.httpStatus, error)) {
        return false;
    }
    if (!response.ok) {
        error = response.error.isEmpty() ? "网络请求失败。" : response.error;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(response.body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = "Codeforces 返回了无法识别的数据。";
        return false;
    }

    const QJsonObject object = document.object();
    if (object.value("status").toString() != "OK") {
        error = object.value("comment").toString("Codeforces API 请求失败。");
        return false;
    }

    result = object.value("result").toArray();
    return true;
}

bool MainWindow::readJsonObject(const OjResponse& response, QJsonObject& result, QString& error) const {
    if (readHttpError(response.httpStatus, error)) {
        return false;
    }
    if (!response.ok) {
        error = response.error.isEmpty() ? "网络请求失败。" : response.error;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(response.body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = "服务返回了无法识别的数据。";
        return false;
    }

    result = document.object();
    return true;
}

void MainWindow::loadCodeforcesAccount(bool forceRefresh) {
    if (codeforcesLoading_) {
        return;
    }

    const QString handle = codeforcesHandleInput_->text().trimmed();
    if (handle.isEmpty()) {
        QMessageBox::information(this, "需要账号", "请输入 Codeforces handle。");
        return;
    }

    codeforcesLoading_ = true;
    codeforcesButton_->setEnabled(false);
    codeforcesVerifyButton_->setEnabled(false);
    codeforcesImportButton_->setEnabled(false);
    solvedCodeforcesProblems_.clear();
    settings_.setValue("codeforces/handle", handle);
    codeforcesSummary_->setText("正在查询账号资料……");
    codeforcesSubmissionsTable_->setRowCount(0);

    ojClient_->get(OjPlatform::Codeforces,
                   codeforcesApiUrl("user.info", "handles", handle),
                   forceRefresh,
                   [this, handle, forceRefresh](const OjResponse& response) {
                       QJsonArray users;
                       QString error;
                       if (!readCodeforcesResult(response, users, error) || users.isEmpty()) {
                           codeforcesLoading_ = false;
                           codeforcesSummary_->setText("查询失败：" + error);
                           codeforcesButton_->setEnabled(true);
                           return;
                       }

                       const QJsonObject user = users.first().toObject();
                       const QString rank = user.value("rank").toString("未定级");
                       const QString rating = user.value("rating").isDouble()
                           ? QString::number(user.value("rating").toInt())
                           : "暂无";
                       const QString maxRating = user.value("maxRating").isDouble()
                           ? QString::number(user.value("maxRating").toInt())
                           : "暂无";
                       codeforcesProfileSummary_ =
                           QString("%1（%2）  当前 rating：%3  最高 rating：%4")
                               .arg(user.value("handle").toString(handle))
                               .arg(rank)
                               .arg(rating)
                               .arg(maxRating);
                       codeforcesSummary_->setText(codeforcesProfileSummary_ +
                                                   "\n正在读取最近提交……");

                       QTimer::singleShot(2100, this, [this, handle, forceRefresh] {
                           loadRecentCodeforcesSubmissions(handle, forceRefresh);
                       });
                   });
}

void MainWindow::loadRecentCodeforcesSubmissions(const QString& handle, bool forceRefresh) {
    ojClient_->get(OjPlatform::Codeforces,
                   codeforcesApiUrl("user.status", "handle", handle),
                   forceRefresh,
                   [this](const OjResponse& response) {
                       QJsonArray submissions;
                       QString error;
                       if (!readCodeforcesResult(response, submissions, error)) {
                           codeforcesLoading_ = false;
                           solvedCodeforcesProblems_.clear();
                           codeforcesSummary_->setText(codeforcesProfileSummary_ +
                                                       "\n提交记录读取失败：" + error);
                           codeforcesButton_->setEnabled(true);
                           return;
                       }

                       QSet<QString> solved;
                       solvedCodeforcesProblems_.clear();
                       for (const QJsonValue& value : submissions) {
                           const QJsonObject submission = value.toObject();
                           if (submission.value("verdict").toString() != "OK") {
                               continue;
                           }
                           const QJsonObject problem = submission.value("problem").toObject();
                           const QString key = codeforcesProblemKey(
                               problem.value("contestId").toInt(),
                               problem.value("index").toString());
                           if (key.isEmpty()) {
                               continue;
                           }
                           solved.insert(key);
                           if (!solvedCodeforcesProblems_.contains(key)) {
                               solvedCodeforcesProblems_.insert(key, problem);
                           }
                       }

                       codeforcesVerifyButton_->setEnabled(
                           !solvedCodeforcesProblems_.isEmpty());
                       codeforcesImportButton_->setEnabled(
                           !solvedCodeforcesProblems_.isEmpty());
                       codeforcesSummary_->setText(
                           codeforcesProfileSummary_ +
                           QString("\n最近 1000 次提交中通过 %1 道不同题目；返回提交数：%2")
                               .arg(solved.size())
                               .arg(submissions.size()));
                       showCodeforcesSubmissions(submissions);
                       codeforcesLoading_ = false;
                       codeforcesButton_->setEnabled(true);
                   });
}

void MainWindow::showCodeforcesSubmissions(const QJsonArray& submissions) {
    codeforcesSubmissionsTable_->setRowCount(0);
    const int first = qMax(0, submissions.size() - 20);
    for (int index = submissions.size() - 1; index >= first; --index) {
        const QJsonObject submission = submissions.at(index).toObject();
        const QJsonObject problem = submission.value("problem").toObject();
        const int row = codeforcesSubmissionsTable_->rowCount();
        codeforcesSubmissionsTable_->insertRow(row);

        const qint64 timestamp = submission.value("creationTimeSeconds").toVariant().toLongLong();
        const QString time = QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd HH:mm");
        const QString problemName = QString("%1 %2")
                                        .arg(problem.value("index").toString(),
                                             problem.value("name").toString());
        const QString verdict = submission.value("verdict").toString("等待评测");
        const QString language = submission.value("programmingLanguage").toString();
        codeforcesSubmissionsTable_->setItem(row, 0, new QTableWidgetItem(time));
        codeforcesSubmissionsTable_->setItem(row, 1, new QTableWidgetItem(problemName));
        codeforcesSubmissionsTable_->setItem(row, 2, new QTableWidgetItem(verdict));
        codeforcesSubmissionsTable_->setItem(row, 3, new QTableWidgetItem(language));
    }
}

void MainWindow::verifyCodeforcesTasks() {
    if (solvedCodeforcesProblems_.isEmpty()) {
        QMessageBox::information(this, "还没有提交数据", "请先查询 Codeforces 账号。");
        return;
    }

    std::vector<int> matches;
    for (std::size_t index = 0; index < tasks_.size(); ++index) {
        const Task& task = tasks_[index];
        const QString platform = QString::fromUtf8(task.platform.c_str()).toLower();
        if (task.completed || platform != "codeforces") {
            continue;
        }
        const QString key = normalizeCodeforcesProblemId(
            QString::fromUtf8(task.problemId.c_str()));
        if (!key.isEmpty() && solvedCodeforcesProblems_.contains(key)) {
            matches.push_back(static_cast<int>(index));
        }
    }

    if (matches.empty()) {
        QMessageBox::information(
            this,
            "没有可核验的任务",
            "没有发现“平台为 Codeforces、已 AC 且尚未完成”的关联任务。\n"
            "请先编辑任务并填写题号，例如 1900A。");
        return;
    }

    rememberState();
    for (int index : matches) {
        tasks_[static_cast<std::size_t>(index)].completed = true;
    }
    saveAndRefresh();
    QMessageBox::information(this,
                             "核验完成",
                             QString("已把 %1 道关联任务标记为完成。").arg(matches.size()));
}

void MainWindow::importCodeforcesSolved() {
    if (solvedCodeforcesProblems_.isEmpty()) {
        QMessageBox::information(this, "还没有提交数据", "请先查询 Codeforces 账号。");
        return;
    }

    QSet<QString> existing;
    for (const Task& task : tasks_) {
        const QString key = normalizeCodeforcesProblemId(
            QString::fromUtf8(task.problemId.c_str()));
        if (!key.isEmpty()) {
            existing.insert(key);
        }
    }

    QStringList keys = solvedCodeforcesProblems_.keys();
    keys.sort();
    std::vector<QString> missing;
    for (const QString& key : keys) {
        if (!existing.contains(key)) {
            missing.push_back(key);
        }
    }

    if (missing.empty()) {
        QMessageBox::information(this, "没有新题", "最近 AC 的题目都已经在任务列表中。");
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        "导入最近 AC",
        QString("将导入 %1 道最近通过、但尚未记录的题目，并标记为完成。\n"
                "题目会带上 Codeforces 平台和题号，之后可用于核验。")
            .arg(static_cast<int>(missing.size())),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    if (answer != QMessageBox::Yes) {
        return;
    }

    rememberState();
    for (const QString& key : missing) {
        const QJsonObject problem = solvedCodeforcesProblems_.value(key);
        const QString name = problem.value("name").toString();
        const int rating = problem.value("rating").toInt(0);

        Task task;
        task.title = toTaskText(name.isEmpty() ? "CF " + key : "CF " + key + " " + name);
        task.category = toTaskText("Codeforces");
        task.difficulty = toTaskText(rating > 0 ? QString::number(rating) : QString());
        task.difficultyRank = difficultyRankFromText(task.difficulty);
        task.platform = "codeforces";
        task.problemId = toTaskText(key);
        task.completed = true;
        task.id = newTaskId();
        task.createdAt = nowIso();
        task.completedAt = task.createdAt;
        tasks_.push_back(task);
    }
    saveAndRefresh();
    QMessageBox::information(this,
                             "导入完成",
                             QString("已导入 %1 道最近 AC 题目。")
                                 .arg(static_cast<int>(missing.size())));
}
