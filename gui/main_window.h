#pragma once

#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLockFile>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStringList>
#include <QTableWidget>
#include <QTabWidget>
#include <QTableView>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "app_paths.h"
#include "codeforces_problem.h"
#include "json_task_store.h"
#include "luogu_problem.h"
#include "oj_client.h"
#include "study_stats.h"
#include "task_export.h"
#include "task_history.h"
#include "task_store.h"
#include "task_table_model.h"
#include "weekly_chart.h"
#include "external_links.h"
#include "style.h"
#include "task_dialog.h"
#include "task_util.h"

#if __has_include("version.h")
#include "version.h"
#else
#define APP_VERSION "0.6.0"
#endif

// 主窗口只保留骨架：数据、控件指针、跨页面的动作。各页面的搭建与处理在 page_*.cpp 里。
class MainWindow : public QWidget {
public:
    explicit MainWindow(app_paths::DataFileInfo dataInfo);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    const std::filesystem::path dataFile_;
    QString dataWarning_;
    std::vector<Task> tasks_;
    TaskHistory history_;
    QNetworkAccessManager* network_;
    OjClient* ojClient_ = nullptr;
    QSettings settings_;
    QAction* undoAction_ = nullptr;
    bool dataLoadFailed_ = false;
    bool codeforcesLoading_ = false;
    bool luoguLoading_ = false;
    QLabel* taskSummary_ = nullptr;
    QCheckBox* pendingOnly_ = nullptr;
    QLineEdit* taskSearch_ = nullptr;
    QLabel* taskStatus_ = nullptr;
    TaskTableModel* taskModel_ = nullptr;
    TaskFilterProxyModel* taskProxy_ = nullptr;
    QTableView* taskTable_ = nullptr;
    QLabel* statsSummary_ = nullptr;
    QLabel* statsDistribution_ = nullptr;
    QLabel* statsUntimed_ = nullptr;
    QLabel* statsDailyLabel_ = nullptr;
    QProgressBar* statsDailyBar_ = nullptr;
    QSpinBox* statsGoalSpin_ = nullptr;
    WeeklyChart* statsWeeklyChart_ = nullptr;
    QLineEdit* codeforcesHandleInput_ = nullptr;
    QPushButton* codeforcesButton_ = nullptr;
    QPushButton* codeforcesRefreshButton_ = nullptr;
    QLabel* codeforcesSummary_ = nullptr;
    QString codeforcesProfileSummary_;
    QPushButton* codeforcesVerifyButton_ = nullptr;
    QPushButton* codeforcesImportButton_ = nullptr;
    QHash<QString, QJsonObject> solvedCodeforcesProblems_;
    QTableWidget* codeforcesSubmissionsTable_ = nullptr;
    QLineEdit* luoguHandleInput_ = nullptr;
    QPushButton* luoguButton_ = nullptr;
    QPushButton* luoguRefreshButton_ = nullptr;
    QLabel* luoguSummary_ = nullptr;
    QLabel* luoguDetails_ = nullptr;
    QLineEdit* luoguProblemInput_ = nullptr;
    QPushButton* luoguProblemButton_ = nullptr;
    QPushButton* luoguProblemRefreshButton_ = nullptr;
    QLabel* luoguProblemInfo_ = nullptr;
    QPushButton* luoguLinkSelectedButton_ = nullptr;
    QPushButton* luoguCreateTaskButton_ = nullptr;
    LuoguProblem luoguProblem_;
    bool luoguProblemLoading_ = false;

    void addPageHeading(QVBoxLayout* layout, const QString& title, const QString& description);
    QFrame* createCard(QWidget* parent);
    QWidget* createStatsPage();
    QWidget* createTasksPage();
    QWidget* createCodeforcesPage();
    QWidget* createLuoguPage();
    void refreshTasks();
    void updateStatsPage();
    void updateTaskSummary();
    void addTask();
    bool taskTitleExists(const QString& title, int ignoredIndex = -1) const;
    void rememberState();
    void updateUndoAction();
    void undoLast();
    int selectedTaskIndex() const;
    void editTask();
    void completeTask();
    void reopenTask();
    void setTaskCompleted(bool completed);
    void deleteTask();
    void restoreBackup();
    void importTasks();
    void exportTasks();
    void saveAndRefresh();
    QUrl codeforcesApiUrl(const QString& method, const QString& parameter, const QString& value) const;
    bool readHttpError(int statusCode, QString& error) const;
    bool readCodeforcesResult(const OjResponse& response, QJsonArray& result, QString& error) const;
    bool readJsonObject(const OjResponse& response, QJsonObject& result, QString& error) const;
    void loadCodeforcesAccount(bool forceRefresh = false);
    void loadRecentCodeforcesSubmissions(const QString& handle, bool forceRefresh = false);
    void showCodeforcesSubmissions(const QJsonArray& submissions);
    void verifyCodeforcesTasks();
    void importCodeforcesSolved();
    void loadLuoguAccount(bool forceRefresh = false);
    void loadLuoguProfile(int uid, bool forceRefresh = false);
    void lookupLuoguProblem(bool forceRefresh = false);
    void linkLuoguProblemToSelectedTask();
    void createTaskFromLuoguProblem();
    void openSelectedProblem();
    void showLuoguError(const QString& error);
};
