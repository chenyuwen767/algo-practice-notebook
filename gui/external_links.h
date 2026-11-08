#pragma once

#include <QString>
#include <QUrl>

namespace taskui {

// 用默认浏览器打开链接。成功返回 true。
// browserUsed 非空时写入实际启动的浏览器路径（走系统关联时置空）。
bool openExternalUrl(const QUrl& url, QString* browserUsed = nullptr);

// 把系统默认浏览器的 ProgId（如 MSEdgeHTM）映射到可执行文件名，认不出返回空串。
QString browserExecutableName(const QString& progId);

// 读取 HKCU 里 https 协议的用户选择；读不到返回空串。
QString defaultBrowserProgId();

// 找到默认浏览器可执行文件的完整路径，找不到返回空串。
QString defaultBrowserExecutable();

// 把可执行文件路径换成给人看的名字：msedge.exe → Edge，chrome.exe → Chrome。
QString browserDisplayName(const QString& executablePath);

} // namespace taskui
