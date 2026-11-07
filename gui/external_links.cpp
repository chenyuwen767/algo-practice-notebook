#include "external_links.h"

#include <QDesktopServices>
#include <QUrl>

namespace taskui {

bool openExternalUrl(const QUrl& url) {
    if (!url.isValid() || url.isEmpty()) {
        return false;
    }
    // 交给系统的 URL 关联处理：不自己拼浏览器命令行，也不创建临时 profile。
    return QDesktopServices::openUrl(url);
}

} // namespace taskui
