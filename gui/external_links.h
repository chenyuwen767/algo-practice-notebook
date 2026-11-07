#pragma once

class QUrl;

namespace taskui {

// 用系统默认浏览器打开链接。失败返回 false，由调用方提示“链接已复制”。
bool openExternalUrl(const QUrl& url);

} // namespace taskui
