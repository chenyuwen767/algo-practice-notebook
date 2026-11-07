#include <QApplication>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QMessageBox>
#include <QPixmap>
#include <QTabWidget>
#include <QTimer>

#include "main_window.h"
#include "task_util.h"

#if __has_include("version.h")
#include "version.h"
#else
#define APP_VERSION "0.6.0"
#endif

using namespace taskui;

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName("chenyuwen");
    QCoreApplication::setApplicationName("AlgorithmPracticeNotebook");
    QCoreApplication::setApplicationVersion(APP_VERSION);

    app_paths::DataFileInfo dataInfo = resolveDataFileInfo();
    const std::filesystem::path dataFile = dataInfo.path;

    auto makeLock = [](const QString& path) {
        return std::make_unique<QLockFile>(path);
    };

    std::unique_ptr<QLockFile> instanceLock = makeLock(QString::fromStdWString(
        (dataFile.parent_path() / "algorithm_notebook.lock").wstring()));
    bool locked = instanceLock->tryLock(1500);

    if (!locked && instanceLock->error() == QLockFile::PermissionError) {
        const QByteArray digest = QCryptographicHash::hash(
            displayPath(dataFile).toUtf8(), QCryptographicHash::Sha256).toHex();
        const QString fallbackPath = QDir::tempPath() + "/algorithm_notebook_" +
                                     QString::fromLatin1(digest.left(16)) + ".lock";
        auto fallbackLock = makeLock(fallbackPath);
        if (fallbackLock->tryLock(500)) {
            instanceLock = std::move(fallbackLock);
            locked = true;
            const QString note = "无法在数据目录创建锁文件，已改用临时锁。";
            dataInfo.warning = dataInfo.warning.isEmpty() ? note : dataInfo.warning + "\n\n" + note;
        }
    }

    if (!locked) {
        QString reason;
        switch (instanceLock->error()) {
        case QLockFile::LockFailedError:
            reason = "另一个实例正在使用同一份任务数据。";
            break;
        case QLockFile::PermissionError:
            reason = "没有权限创建锁文件，请检查数据目录权限。";
            break;
        default:
            reason = "无法创建锁文件。";
            break;
        }
        QMessageBox::warning(
            nullptr,
            "无法启动",
            QString("无法独占任务数据文件：\n%1\n\n锁文件：\n%2\n\n%3")
                .arg(displayPath(dataFile), instanceLock->fileName(), reason));
        return 1;
    }

    MainWindow window(dataInfo);
    window.show();

    // Used by CI and local smoke checks: construct the real window, optionally
    // save a screenshot, then quit. 退出码非 0 表示窗口没渲染出来。
    if (QCoreApplication::arguments().contains("--smoke-test")) {
        QTimer::singleShot(700, &application, [&application, &window] {
            if (QCoreApplication::arguments().contains("--smoke-stats") &&
                window.findChild<QTabWidget*>() != nullptr) {
                window.findChild<QTabWidget*>()->setCurrentIndex(1);
            }
            // 视口里选中第一行，用来验证练习记录面板确实跟着选中走。
            int selectRow = -1;
            for (const QString& argument : QCoreApplication::arguments()) {
                if (argument.startsWith("--smoke-select")) {
                    selectRow = 0;
                    const QString suffix = argument.section('=', 1);
                    if (!suffix.isEmpty()) {
                        selectRow = suffix.toInt();
                    }
                }
            }
            if (selectRow >= 0 && window.findChild<QTableView*>("taskTable") != nullptr) {
                QTableView* table = window.findChild<QTableView*>("taskTable");
                if (table->model() != nullptr && selectRow < table->model()->rowCount()) {
                    table->selectRow(selectRow);
                    table->setCurrentIndex(table->model()->index(selectRow, 0));
                }
            }
            const QPixmap shot = window.grab();
            // 优先写程序目录（CI 直接从 build/ 收集产物），临时目录作为兜底。
            const QString primary = QCoreApplication::applicationDirPath() + "/smoke.png";
            const QString secondary = QDir::tempPath() + "/algorithm_notebook_smoke.png";
            QString shotPath = primary;
            bool saved = !shot.isNull() && shot.save(primary);
            if (!saved && !shot.isNull()) {
                shotPath = secondary;
                saved = shot.save(secondary);
            }
            QFile log(QCoreApplication::applicationDirPath() + "/smoke.log");
            if (log.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                log.write(QString("grab=%1 size=%2x%3 saved=%4 shot=%5 tempPath=%6\n")
                              .arg(!shot.isNull())
                              .arg(shot.width())
                              .arg(shot.height())
                              .arg(saved)
                              .arg(shotPath, QDir::tempPath())
                              .toUtf8());
            }
            QTimer::singleShot(300, &application, &QCoreApplication::quit);
            QCoreApplication::exit(saved ? 0 : 1);
        });
    }

    return application.exec();
}