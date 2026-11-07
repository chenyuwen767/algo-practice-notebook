#include "main_window.h"

using namespace taskui;

MainWindow::MainWindow(app_paths::DataFileInfo dataInfo)
    : dataFile_(dataInfo.path),
      dataWarning_(dataInfo.warning),
      network_(new QNetworkAccessManager(this)) {
    ojClient_ = new OjClient(network_, this);
    const JsonLoadResult loaded = loadTasksJson(dataFile_);
    tasks_ = loaded.tasks;
    ensureTaskIds(tasks_);
    dataLoadFailed_ = !loaded.ok || !loaded.warnings.empty();

    QString loadError = QString::fromStdString(loaded.error);
    QStringList loadWarnings;
    for (const std::string& warning : loaded.warnings) {
        loadWarnings << QString::fromStdString(warning);
    }

    if (loaded.ok && !dataInfo.legacyPath.empty()) {
        const LoadResult legacy = loadTasksChecked(dataInfo.legacyPath);
        if (!legacy.ok) {
            dataLoadFailed_ = true;
            loadError = QString("旧数据迁移失败：%1\n\n%2")
                            .arg(QString::fromStdString(legacy.error),
                                 displayPath(dataInfo.legacyPath));
        } else {
            tasks_ = legacy.tasks;
            ensureTaskIds(tasks_);
            for (const std::string& warning : legacy.warnings) {
                loadWarnings << QString::fromStdString(warning);
            }
            if (!legacy.warnings.empty()) {
                dataLoadFailed_ = true;
            }

            std::string saveError;
            if (!saveTasksJson(tasks_, dataFile_, saveError)) {
                dataLoadFailed_ = true;
                loadError = QString("无法写入迁移后的 tasks.json：%1")
                                .arg(QString::fromStdString(saveError));
            } else {
                const QString notice =
                    QString("已从旧文件迁移到 tasks.json：\n%1\n\n旧文件保留未删除。")
                        .arg(displayPath(dataInfo.legacyPath));
                dataWarning_ = dataWarning_.isEmpty() ? notice : dataWarning_ + "\n\n" + notice;
            }
        }
    }

    setWindowTitle(QString("算法练习任务本 v%1").arg(APP_VERSION));
    resize(960, 680);
    const QByteArray geometry = settings_.value("window/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    setStyleSheet(QString::fromUtf8(appStyleSheet()));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 18, 22, 22);
    layout->setSpacing(14);

    auto* menuBar = new QMenuBar(this);
    auto* fileMenu = menuBar->addMenu("文件");
    auto* openDataAction = fileMenu->addAction("打开数据目录");
    QObject::connect(openDataAction, &QAction::triggered, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(displayPath(dataFile_.parent_path())));
    });
    auto* exportAction = fileMenu->addAction("导出为 CSV…");
    QObject::connect(exportAction, &QAction::triggered, this, [this] { exportTasks(); });
    auto* importAction = fileMenu->addAction("从 CSV 导入…");
    QObject::connect(importAction, &QAction::triggered, this, [this] { importTasks(); });
    auto* restoreAction = fileMenu->addAction("从备份恢复…");
    QObject::connect(restoreAction, &QAction::triggered, this, [this] { restoreBackup(); });
    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction("退出");
    QObject::connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto* editMenu = menuBar->addMenu("编辑");
    undoAction_ = editMenu->addAction("撤销上一步");
    undoAction_->setEnabled(false);
    QObject::connect(undoAction_, &QAction::triggered, this, [this] { undoLast(); });

    auto* helpMenu = menuBar->addMenu("帮助");
    auto* aboutAction = helpMenu->addAction("关于");
    QObject::connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::about(
            this,
            "关于算法练习任务本",
            QString("算法练习任务本 %1\n\n数据文件：%2\n\n"
                    "仅读取 Codeforces 与洛谷的公开数据，不保存或传输账号密码。")
                .arg(QCoreApplication::applicationVersion(), displayPath(dataFile_)));
    });
    layout->setMenuBar(menuBar);

    auto* title = new QLabel("算法练习任务本", this);
    title->setObjectName("appTitle");
    auto* subtitle = new QLabel("集中记录练习任务，并查看在线评测平台的公开进度", this);
    subtitle->setObjectName("pageDescription");
    layout->addWidget(title);
    layout->addWidget(subtitle);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(createTasksPage(), "练习任务");
    tabs->addTab(createStatsPage(), "学习统计");
    tabs->addTab(createCodeforcesPage(), "Codeforces");
    tabs->addTab(createLuoguPage(), "洛谷");
    layout->addWidget(tabs);

    codeforcesHandleInput_->setText(settings_.value("codeforces/handle").toString());
    luoguHandleInput_->setText(settings_.value("luogu/handle").toString());
    pendingOnly_->setChecked(settings_.value("tasks/pendingOnly", false).toBool());
    refreshTasks();

    if (!loadError.isEmpty()) {
        QTimer::singleShot(0, this, [this, loadError] {
            QMessageBox::critical(
                this,
                "数据读取失败",
                QString("无法读取任务数据：%1\n\n路径：%2\n\n为避免覆盖现有数据，继续保存前会再次确认。")
                    .arg(loadError, displayPath(dataFile_)));
        });
    } else if (!loadWarnings.isEmpty()) {
        QTimer::singleShot(0, this, [this, text = loadWarnings.join('\n')] {
            QMessageBox::warning(
                this,
                "部分数据未读取",
                text + "\n\n继续修改并保存会以当前列表为准，被跳过的条目无法恢复。");
        });
    }

    if (!dataWarning_.isEmpty()) {
        QTimer::singleShot(0, this, [this] {
            QMessageBox::warning(
                this,
                "数据位置提示",
                dataWarning_ + "\n\n当前数据文件：\n" + displayPath(dataFile_));
        });
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    settings_.setValue("window/geometry", saveGeometry());
    QWidget::closeEvent(event);
}

void MainWindow::addPageHeading(QVBoxLayout* layout, const QString& title, const QString& description) {
    auto* heading = new QLabel(title);
    heading->setObjectName("pageTitle");
    auto* note = new QLabel(description);
    note->setObjectName("pageDescription");
    note->setWordWrap(true);
    layout->addWidget(heading);
    layout->addWidget(note);
}

QFrame* MainWindow::createCard(QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName("card");
    return card;
}

void MainWindow::rememberState() {
    history_.record(tasks_);
    updateUndoAction();
}

void MainWindow::updateUndoAction() {
    if (undoAction_) {
        undoAction_->setEnabled(history_.canUndo());
    }
}

void MainWindow::undoLast() {
    std::vector<Task> previous;
    if (!history_.undo(previous)) {
        QMessageBox::information(this, "没有可撤销的操作", "已经回到本轮最早的状态。");
        return;
    }
    tasks_ = previous;
    updateUndoAction();
    saveAndRefresh();
}

void MainWindow::saveAndRefresh() {
    if (dataLoadFailed_) {
        const auto answer = QMessageBox::warning(
            this,
            "数据尚未成功读取",
            "启动时未能读取原任务数据。继续保存会用当前列表覆盖数据文件。\n\n是否继续？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    std::string error;
    if (!saveTasksJson(tasks_, dataFile_, error)) {
        QMessageBox::critical(
            this,
            "保存失败",
            QString("无法写入任务数据：%1\n\n路径：%2")
                .arg(QString::fromStdString(error), displayPath(dataFile_)));
        return;
    }

    dataLoadFailed_ = false;
    refreshTasks();
}

int MainWindow::selectedTaskIndex() const {
    const QModelIndexList selected = taskTable_->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        const QModelIndex sourceIndex = taskProxy_->mapToSource(selected.first());
        if (sourceIndex.isValid()) {
            return sourceIndex.row();
        }
    }

    const QModelIndex proxyIndex = taskTable_->currentIndex();
    if (!proxyIndex.isValid()) {
        return -1;
    }
    const QModelIndex sourceIndex = taskProxy_->mapToSource(proxyIndex);
    return sourceIndex.isValid() ? sourceIndex.row() : -1;
}

bool MainWindow::taskTitleExists(const QString& title, int ignoredIndex) const {
    for (std::size_t index = 0; index < tasks_.size(); ++index) {
        if (static_cast<int>(index) == ignoredIndex) {
            continue;
        }
        if (taskText(tasks_[index].title).compare(title, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void MainWindow::openSelectedProblem() {
    const int index = selectedTaskIndex();
    if (index < 0) {
        QMessageBox::information(this, "请选择题目", "请先在任务表格中选择一道题目。");
        return;
    }

    const Task& task = tasks_[static_cast<std::size_t>(index)];
    const QString platform = taskText(task.platform).toLower();
    const QString problemId = taskText(task.problemId);
    QUrl url;
    if (platform == "codeforces") {
        url = codeforcesProblemUrl(problemId);
    } else if (platform == "luogu") {
        url = luoguProblemUrl(problemId);
    } else {
        url = codeforcesProblemUrl(problemId);
        if (url.isEmpty()) {
            url = luoguProblemUrl(problemId);
        }
    }

    if (url.isEmpty()) {
        QMessageBox::information(
            this,
            "没有题目链接",
            "这道题还没有有效的平台/题号。请先编辑任务并填写平台和题号。");
        return;
    }

    QApplication::clipboard()->setText(url.toString());
    if (!openExternalUrl(url)) {
        QMessageBox::warning(
            this,
            "无法打开浏览器",
            "未能启动系统浏览器。链接已复制到剪贴板，请手动粘贴访问：\n" +
                url.toString());
        return;
    }
    taskStatus_->setText("已在浏览器打开，链接同时复制到剪贴板：" + url.toString());
    taskStatus_->show();
    QTimer::singleShot(6000, this, [this] {
        if (taskStatus_ != nullptr) {
            taskStatus_->hide();
        }
    });
}
