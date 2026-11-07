#include "external_links.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStringList>
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

bool openExternalUrl(const QUrl& url) {
    if (!url.isValid() || url.isEmpty()) {
        return false;
    }
#ifdef Q_OS_WIN
    // 直接用浏览器的可执行文件打开：有些机器上 Edge 是 UWP 应用，ShellExecute 走 URL
    // 关联会“返回成功但没有启动”，用户看到的就是点了没反应。
    const QString target = url.toString();
    const QString defaultBrowser = defaultBrowserExecutable();
    if (!defaultBrowser.isEmpty() && QProcess::startDetached(defaultBrowser, {target})) {
        return true;
    }
    // 默认浏览器认不出来或起不来：退回系统关联。
    if (QDesktopServices::openUrl(url)) {
        return true;
    }
    // 最后一层：机器上装着别的浏览器就试一个，总比什么都不发生强。
    for (const QString& exeName : {QStringLiteral("chrome.exe"),
                                   QStringLiteral("msedge.exe"),
                                   QStringLiteral("firefox.exe")}) {
        const QString browser = resolveExecutable(exeName);
        if (browser.isEmpty() || browser == defaultBrowser) {
            continue;
        }
        if (QProcess::startDetached(browser, {target})) {
            return true;
        }
    }
    return false;
#else
    return QDesktopServices::openUrl(url);
#endif
}

} // namespace taskui
