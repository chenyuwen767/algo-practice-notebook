#include "main_window.h"

using namespace taskui;

QWidget* MainWindow::createTasksPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    addPageHeading(layout, "我的练习任务", "把算法题目整理成可持续推进的练习清单。");

    auto* card = createCard(page);
    auto* cardLayout = new QVBoxLayout(card);
    taskSummary_ = new QLabel(card);
    taskSummary_->setObjectName("summaryCard");
    pendingOnly_ = new QCheckBox("只显示待完成题目", card);
    cardLayout->addWidget(taskSummary_);
    cardLayout->addWidget(pendingOnly_);
    layout->addWidget(card);

    auto* searchCard = createCard(page);
    auto* searchLayout = new QHBoxLayout(searchCard);
    auto* searchLabel = new QLabel("搜索", searchCard);
    taskSearch_ = new QLineEdit(searchCard);
    taskSearch_->setClearButtonEnabled(true);
    taskSearch_->setPlaceholderText("题目名称、平台/题号、分类、难度、标签或备注");
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(taskSearch_);
    layout->addWidget(searchCard);

    taskModel_ = new TaskTableModel(this);
    taskModel_->setTasks(&tasks_);
    taskProxy_ = new TaskFilterProxyModel(this);
    taskProxy_->setSourceModel(taskModel_);

    taskTable_ = new QTableView(page);
    taskTable_->setModel(taskProxy_);
    taskTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    taskTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    taskTable_->setAlternatingRowColors(true);
    taskTable_->setSortingEnabled(true);
    taskTable_->sortByColumn(TaskTableModel::TitleColumn, Qt::AscendingOrder);
    taskTable_->horizontalHeader()->setStretchLastSection(true);
    taskTable_->horizontalHeader()->setSectionResizeMode(TaskTableModel::TitleColumn,
                                                         QHeaderView::Stretch);
    taskTable_->horizontalHeader()->setSectionResizeMode(TaskTableModel::ProblemColumn,
                                                         QHeaderView::ResizeToContents);
    taskTable_->verticalHeader()->setVisible(false);
    layout->addWidget(taskTable_);

    auto* buttons = new QHBoxLayout;
    auto* addButton = new QPushButton("新增题目", page);
    auto* editButton = new QPushButton("编辑题目", page);
    auto* openButton = new QPushButton("打开题目", page);
    auto* completeButton = new QPushButton("标记为完成", page);
    auto* pendingButton = new QPushButton("标记为待完成", page);
    auto* deleteButton = new QPushButton("删除题目", page);
    deleteButton->setObjectName("dangerButton");
    buttons->addWidget(addButton);
    buttons->addWidget(editButton);
    buttons->addWidget(openButton);
    buttons->addWidget(completeButton);
    buttons->addWidget(pendingButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    taskStatus_ = new QLabel(page);
    taskStatus_->setObjectName("hint");
    taskStatus_->setWordWrap(true);
    taskStatus_->hide();
    layout->addWidget(taskStatus_);

    connect(pendingOnly_, &QCheckBox::toggled, this, [this](bool checked) {
        settings_.setValue("tasks/pendingOnly", checked);
        taskProxy_->setPendingOnly(checked);
        updateTaskSummary();
    });
    connect(taskSearch_, &QLineEdit::textChanged, this, [this](const QString& text) {
        taskProxy_->setKeyword(text.trimmed());
        updateTaskSummary();
    });
    connect(addButton, &QPushButton::clicked, this, [this] { addTask(); });
    connect(editButton, &QPushButton::clicked, this, [this] { editTask(); });
    connect(openButton, &QPushButton::clicked, this, [this] { openSelectedProblem(); });
    connect(completeButton, &QPushButton::clicked, this, [this] { completeTask(); });
    connect(pendingButton, &QPushButton::clicked, this, [this] { reopenTask(); });
    connect(deleteButton, &QPushButton::clicked, this, [this] { deleteTask(); });
    connect(taskTable_, &QTableView::doubleClicked, this, [this](const QModelIndex&) {
        editTask();
    });

    auto* newShortcut = new QShortcut(QKeySequence::New, page);
    newShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(newShortcut, &QShortcut::activated, this, [this] { addTask(); });
    auto* findShortcut = new QShortcut(QKeySequence::Find, page);
    findShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(findShortcut, &QShortcut::activated, this, [this] {
        taskSearch_->setFocus();
        taskSearch_->selectAll();
    });
    auto* openShortcut = new QShortcut(QKeySequence("Ctrl+O"), page);
    openShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(openShortcut, &QShortcut::activated, this, [this] { openSelectedProblem(); });
    auto* deleteShortcut = new QShortcut(QKeySequence::Delete, taskTable_);
    deleteShortcut->setContext(Qt::WidgetShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, [this] { deleteTask(); });
    auto* undoShortcut = new QShortcut(QKeySequence::Undo, taskTable_);
    undoShortcut->setContext(Qt::WidgetShortcut);
    connect(undoShortcut, &QShortcut::activated, this, [this] { undoLast(); });
    return page;
}

void MainWindow::refreshTasks() {
    taskModel_->reload();
    taskProxy_->setKeyword(taskSearch_ ? taskSearch_->text().trimmed() : QString());
    taskProxy_->setPendingOnly(pendingOnly_->isChecked());
    updateTaskSummary();
    updateStatsPage();
}

void MainWindow::updateTaskSummary() {
    int completed = 0;
    for (const Task& task : tasks_) {
        if (task.completed) {
            ++completed;
        }
    }

    const int visible = taskProxy_->rowCount();
    QString summary = QString("共 %1 道题，已完成 %2 道（%3%）")
                          .arg(tasks_.size())
                          .arg(completed)
                          .arg(tasks_.empty() ? 0
                                              : completed * 100 / static_cast<int>(tasks_.size()));
    if (visible != static_cast<int>(tasks_.size())) {
        summary += QString("，当前显示 %1 道").arg(visible);
    }
    taskSummary_->setText(summary);
}

void MainWindow::addTask() {
    QString title;
    QString category;
    QString difficulty;
    QString platform;
    QString problemId;
    QString tags;
    QString notes;
    if (!promptTaskFields(this, "新增题目", title, category, difficulty, platform, problemId,
                          tags, notes)) {
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
}

void MainWindow::editTask() {
    const int index = selectedTaskIndex();
    if (index < 0) {
        QMessageBox::information(this, "请选择题目", "请先在表格中选择一道题目。");
        return;
    }

    const Task& task = tasks_[static_cast<std::size_t>(index)];
    QString title = taskText(task.title);
    QString category = taskText(task.category);
    QString difficulty = taskText(task.difficulty);
    QString platform = taskText(task.platform);
    QString problemId = taskText(task.problemId);
    QString tags = tagsText(task.tags);
    QString notes = taskText(task.notes);
    if (!promptTaskFields(this, "编辑题目", title, category, difficulty, platform, problemId,
                          tags, notes)) {
        return;
    }
    if (taskTitleExists(title, index)) {
        const auto answer = QMessageBox::question(
            this,
            "题目已存在",
            QString("“%1”已经出现在任务列表中，仍要保存吗？").arg(title),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    rememberState();
    tasks_[static_cast<std::size_t>(index)].title = toTaskText(title);
    tasks_[static_cast<std::size_t>(index)].category = toTaskText(category);
    tasks_[static_cast<std::size_t>(index)].difficulty = toTaskText(difficulty);
    tasks_[static_cast<std::size_t>(index)].platform = toTaskText(platform);
    tasks_[static_cast<std::size_t>(index)].problemId = toTaskText(problemId);
    tasks_[static_cast<std::size_t>(index)].tags = toTaskTags(tags);
    tasks_[static_cast<std::size_t>(index)].notes = toTaskText(notes);
    if (tasks_[static_cast<std::size_t>(index)].id.empty()) {
        tasks_[static_cast<std::size_t>(index)].id = newTaskId();
    }
    saveAndRefresh();
}

void MainWindow::completeTask() {
    setTaskCompleted(true);
}

void MainWindow::reopenTask() {
    setTaskCompleted(false);
}

void MainWindow::setTaskCompleted(bool completed) {
    const int index = selectedTaskIndex();
    if (index < 0) {
        QMessageBox::information(this, "请选择题目", "请先在表格中选择一道题目。");
        return;
    }
    if (tasks_[static_cast<std::size_t>(index)].completed == completed) {
        return;
    }
    rememberState();
    Task& task = tasks_[static_cast<std::size_t>(index)];
    task.completed = completed;
    if (completed) {
        if (task.completedAt.empty()) {
            task.completedAt = nowIso();
        }
    } else {
        task.completedAt.clear();
    }
    saveAndRefresh();
}

void MainWindow::deleteTask() {
    const int index = selectedTaskIndex();
    if (index < 0) {
        QMessageBox::information(this, "请选择题目", "请先在表格中选择一道题目。");
        return;
    }

    const QString title = taskText(tasks_[static_cast<std::size_t>(index)].title);
    const auto answer = QMessageBox::question(
        this,
        "删除题目",
        QString("确定删除“%1”吗？\n\n此操作不可撤销。").arg(title),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    rememberState();
    tasks_.erase(tasks_.begin() + index);
    saveAndRefresh();
}

void MainWindow::restoreBackup() {
    const QString mainFile = displayPath(dataFile_);
    const QString backupFile = mainFile + ".bak";
    if (!QFile::exists(backupFile)) {
        QMessageBox::information(this, "没有备份", "当前还没有可用的 tasks.json.bak 备份。");
        return;
    }
    const auto answer = QMessageBox::question(
        this,
        "从备份恢复",
        QString("将用备份覆盖当前任务数据：\n%1\n\n是否继续？").arg(backupFile),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    rememberState();
    QFile backup(backupFile);
    if (!backup.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "恢复失败", backup.errorString());
        return;
    }
    const QByteArray data = backup.readAll();

    QSaveFile output(mainFile);
    if (!output.open(QIODevice::WriteOnly) || output.write(data) != data.size() || !output.commit()) {
        QMessageBox::critical(this, "恢复失败", output.errorString());
        return;
    }

    const JsonLoadResult loaded = loadTasksJson(dataFile_);
    tasks_ = loaded.tasks;
    ensureTaskIds(tasks_);
    dataLoadFailed_ = !loaded.ok || !loaded.warnings.empty();
    refreshTasks();
    QMessageBox::information(this, "恢复完成", "已从备份恢复任务数据。");
}

void MainWindow::importTasks() {
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        "导入任务",
        QDir::homePath(),
        "CSV 文件 (*.csv)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "导入失败", file.errorString());
        return;
    }

    std::vector<Task> imported;
    std::string error;
    if (!tasksFromCsv(file.readAll().toStdString(), imported, error)) {
        QMessageBox::critical(this, "导入失败", QString::fromStdString(error));
        return;
    }
    ensureTaskIds(imported);
    if (imported.empty()) {
        QMessageBox::information(this, "没有任务", "CSV 中没有可导入的任务。");
        return;
    }

    QMessageBox box(
        QMessageBox::Question,
        "导入任务",
        QString("读取到 %1 条任务。\n\n追加到现有列表，还是替换现有列表？").arg(imported.size()),
        QMessageBox::NoButton,
        this);
    auto* appendButton = box.addButton("追加", QMessageBox::AcceptRole);
    auto* replaceButton = box.addButton("替换", QMessageBox::DestructiveRole);
    box.addButton("取消", QMessageBox::RejectRole);
    box.exec();

    if (box.clickedButton() == appendButton) {
        rememberState();
        tasks_.insert(tasks_.end(), imported.begin(), imported.end());
    } else if (box.clickedButton() == replaceButton) {
        rememberState();
        tasks_ = imported;
    } else {
        return;
    }
    saveAndRefresh();
}

void MainWindow::exportTasks() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出任务",
        QDir::homePath() + "/algorithm-tasks.csv",
        "CSV 文件 (*.csv)");
    if (fileName.isEmpty()) {
        return;
    }
    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        fileName += ".csv";
    }

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "导出失败", file.errorString());
        return;
    }

    const QByteArray data = QByteArray::fromStdString(tasksToCsv(tasks_));
    if (file.write(data) != data.size() || !file.commit()) {
        QMessageBox::critical(this, "导出失败", file.errorString());
        return;
    }

    QMessageBox::information(
        this,
        "导出完成",
        QString("已导出 %1 条任务到：\n%2").arg(tasks_.size()).arg(fileName));
}
