#include "external_links.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QUrl>

namespace taskui {
namespace {

QString executableFromAppPaths(const QString& exeName) {
    const QStringList roots = {
        QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\"),
    };
    for (const QString& root : roots) {
        QSettings key(root + exeName, QSettings::NativeFormat);
        // App Paths 下每个子键的默认值就是可执行文件路径。
        const QString path = key.value(QStringLiteral("Default")).toString();
        if (!path.isEmpty() && QFileInfo::exists(path)) {
            return QDir::toNativeSeparators(path);
        }
    }
    return {};
}

QString executableFromKnownLocations(const QString& exeName) {
    const QString programFiles = qEnvironmentVariable("ProgramFiles");
    const QString programFilesX86 = qEnvironmentVariable("ProgramFiles(x86)");
    const QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    const QStringList candidates = {
        programFilesX86 + "/Microsoft/Edge/Application/" + exeName,
        programFiles + "/Microsoft/Edge/Application/" + exeName,
        programFiles + "/Google/Chrome/Application/" + exeName,
        programFilesX86 + "/Google/Chrome/Application/" + exeName,
        localAppData + "/Google/Chrome/Application/" + exeName,
        programFiles + "/Mozilla Firefox/" + exeName,
        programFilesX86 + "/Mozilla Firefox/" + exeName,
        programFiles + "/BraveSoftware/Brave-Browser/Application/" + exeName,
        localAppData + "/Programs/Opera/" + exeName,
    };
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::toNativeSeparators(candidate);
        }
    }
    return {};
}

QString resolveExecutable(const QString& exeName) {
    if (exeName.isEmpty()) {
        return {};
    }
    const QString fromAppPaths = executableFromAppPaths(exeName);
    if (!fromAppPaths.isEmpty()) {
        return fromAppPaths;
    }
    return executableFromKnownLocations(exeName);
}

// 浏览器启动过程写一行日志，出问题时用户可以把这份日志发过来。
void appendDiagnostic(const QString& line) {
    const QStringList dirs = {
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        QStandardPaths::writableLocation(QStandardPaths::TempLocation),
    };
    for (const QString& dir : dirs) {
        if (dir.isEmpty()) {
            continue;
        }
        QDir().mkpath(dir);
        QFile file(QDir(dir).filePath(QStringLiteral("browser-open.log")));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            continue;
        }
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << ' ' << line << '\n';
        return;
    }
}

// 浏览器通常把 URL 交给已经运行的实例后自己退出（退出码 0）；起不来时会在几百毫秒内
// 以非零码退出（例如 Edge 的默认配置目录不可写时退出码是 21）。靠这个差别判断
// “到底有没有把页面交出去”，ShellExecute 的返回值在这里没有参考价值。
bool launchAndVerify(const QString& exe, const QUrl& url) {
    QProcess process;
    process.setProgram(exe);
    process.setArguments({url.toString()});
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    process.start(QIODevice::NotOpen);
    if (!process.waitForStarted(4000)) {
        appendDiagnostic(QStringLiteral("start failed: ") + exe + " (" + process.errorString() + ')');
        return false;
    }
    if (process.waitForFinished(800)) {
        const bool ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
        appendDiagnostic(QStringLiteral("exited: ") + exe + " code=" + QString::number(process.exitCode()) +
                         (ok ? " (handed off)" : " (failed)"));
        return ok;
    }
    appendDiagnostic(QStringLiteral("running: ") + exe);
    return true;
}

} // namespace

QString browserExecutableName(const QString& progId) {
    const QString id = progId.trimmed().toLower();
    if (id.isEmpty()) {
        return {};
    }
    if (id.startsWith(QStringLiteral("msedge"))) {
        return QStringLiteral("msedge.exe");
    }
    if (id.startsWith(QStringLiteral("chrome"))) {
        return QStringLiteral("chrome.exe");
    }
    if (id.startsWith(QStringLiteral("firefox"))) {
        return QStringLiteral("firefox.exe");
    }
    if (id.startsWith(QStringLiteral("brave"))) {
        return QStringLiteral("brave.exe");
    }
    if (id.startsWith(QStringLiteral("opera"))) {
        return QStringLiteral("opera.exe");
    }
    return {};
}

QString defaultBrowserProgId() {
    QSettings key(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\Associations\\"
                                 "UrlAssociations\\https\\UserChoice"),
                  QSettings::NativeFormat);
    return key.value(QStringLiteral("ProgId")).toString().trimmed();
}

QString defaultBrowserExecutable() {
    return resolveExecutable(browserExecutableName(defaultBrowserProgId()));
}

QString browserDisplayName(const QString& executablePath) {
    const QString name = QFileInfo(executablePath).completeBaseName().toLower();
    if (name == QStringLiteral("msedge")) {
        return QStringLiteral("Edge");
    }
    if (name == QStringLiteral("chrome")) {
        return QStringLiteral("Chrome");
    }
    if (name == QStringLiteral("firefox")) {
        return QStringLiteral("Firefox");
    }
    if (name == QStringLiteral("brave")) {
        return QStringLiteral("Brave");
    }
    if (name == QStringLiteral("opera")) {
        return QStringLiteral("Opera");
    }
    return name;
}

bool openExternalUrl(const QUrl& url, QString* browserUsed) {
    if (!url.isValid() || url.isEmpty()) {
        return false;
    }
#ifdef Q_OS_WIN
    // 依次尝试：默认浏览器 → 其它已安装的浏览器 → 系统关联。
    // 每一步都确认进程真的起来了（或者把链接交给了已在运行的实例），
    // 因为 ShellExecute 对“默认浏览器启动失败”也会返回成功。
    QStringList candidates;
    const QString defaultBrowser = defaultBrowserExecutable();
    if (!defaultBrowser.isEmpty()) {
        candidates.append(defaultBrowser);
    }
    for (const QString& exeName : {QStringLiteral("chrome.exe"),
                                   QStringLiteral("msedge.exe"),
                                   QStringLiteral("firefox.exe")}) {
        const QString browser = resolveExecutable(exeName);
        if (!browser.isEmpty() && !candidates.contains(browser)) {
            candidates.append(browser);
        }
    }

    appendDiagnostic(QStringLiteral("open ") + url.toString() + " candidates=" +
                     QString::number(candidates.size()));

    // 先试默认浏览器：健康时它最快，而且保留登录态。
    if (!defaultBrowser.isEmpty() && launchAndVerify(defaultBrowser, url)) {
        if (browserUsed != nullptr) {
            *browserUsed = defaultBrowser;
        }
        return true;
    }
    // 默认浏览器起不来（例如 Edge 的配置目录坏了会以退出码 21 结束）时，
    // 交给系统关联：Windows 会按它自己的规则挑一个能用的浏览器。
    appendDiagnostic(QStringLiteral("trying ShellExecute"));
    if (QDesktopServices::openUrl(url)) {
        appendDiagnostic(QStringLiteral("ShellExecute returned true"));
        if (browserUsed != nullptr) {
            browserUsed->clear();
        }
        return true;
    }
    // 系统关联也失败：挨个直接启动别的浏览器。
    for (const QString& browser : candidates) {
        if (browser == defaultBrowser) {
            continue;
        }
        if (launchAndVerify(browser, url)) {
            if (browserUsed != nullptr) {
                *browserUsed = browser;
            }
            return true;
        }
    }
    return false;
#else
    Q_UNUSED(browserUsed);
    return QDesktopServices::openUrl(url);
#endif
}

} // namespace taskui
