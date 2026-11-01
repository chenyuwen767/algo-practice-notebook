#include "oj_client.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace {

QString cacheKey(OjPlatform platform, const QUrl& url) {
    return QString::number(static_cast<int>(platform)) + "|" +
           url.toString(QUrl::FullyEncoded);
}

} // namespace

bool OjCache::lookup(const QString& key, const QDateTime& now, OjResponse& response) const {
    const auto it = entries_.constFind(key);
    if (it == entries_.constEnd()) {
        return false;
    }
    if (it->storedAt.secsTo(now) > ttlSeconds) {
        return false;
    }

    response.ok = it->status == 200;
    response.fromCache = true;
    response.httpStatus = it->status;
    response.body = it->body;
    return true;
}

void OjCache::store(const QString& key,
                    const QByteArray& body,
                    int status,
                    const QDateTime& now) {
    if (status != 200) {
        return;
    }
    entries_.insert(key, Entry{body, status, now});
}

void OjCache::clear() {
    entries_.clear();
}

OjClient::OjClient(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), network_(network) {}

OjClient::~OjClient() {
    queue_.clear();
    cache_.clear();
    if (active_ != nullptr) {
        active_->disconnect(this);
        active_->abort();
        active_ = nullptr;
    }
}

int OjClient::minimumIntervalMs(OjPlatform platform) {
    return platform == OjPlatform::Codeforces ? 2000 : 1000;
}

void OjClient::get(OjPlatform platform, const QUrl& url, bool forceRefresh, Callback callback) {
    const QString key = cacheKey(platform, url);
    if (!forceRefresh) {
        OjResponse cached;
        if (cache_.lookup(key, QDateTime::currentDateTime(), cached)) {
            QTimer::singleShot(0, this, [callback = std::move(callback), cached] {
                if (callback) {
                    callback(cached);
                }
            });
            return;
        }
    }

    Request request;
    request.platform = platform;
    request.url = url;
    request.callback = std::move(callback);
    queue_.enqueue(std::move(request));
    startNext();
}

void OjClient::startNext() {
    if (active_ != nullptr || waiting_ || queue_.isEmpty()) {
        return;
    }

    Request request = queue_.head();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 lastStarted = lastStartedMs_.value(static_cast<int>(request.platform), 0);
    const qint64 waitMs = minimumIntervalMs(request.platform) - (now - lastStarted);
    if (waitMs > 0) {
        waiting_ = true;
        QTimer::singleShot(static_cast<int>(waitMs), this, [this] {
            waiting_ = false;
            startNext();
        });
        return;
    }

    queue_.dequeue();
    lastStartedMs_[static_cast<int>(request.platform)] = now;

    QNetworkRequest networkRequest(request.url);
    networkRequest.setTransferTimeout(15000);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, "AlgorithmPracticeNotebook/0.6");

    QNetworkReply* reply = network_->get(networkRequest);
    active_ = reply;
    const QString key = cacheKey(request.platform, request.url);

    connect(reply, &QNetworkReply::finished, this, [this, reply, request, key] {
        const int statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

        if (statusCode == 429 && request.retryCount < 1) {
            active_ = nullptr;
            reply->deleteLater();
            Request retry = request;
            ++retry.retryCount;
            queue_.prepend(std::move(retry));
            waiting_ = true;
            QTimer::singleShot(5000, this, [this] {
                waiting_ = false;
                startNext();
            });
            return;
        }

        OjResponse response;
        response.httpStatus = statusCode;
        response.body = body;
        response.ok = statusCode == 200 && reply->error() == QNetworkReply::NoError;
        if (!response.ok) {
            response.error = reply->error() == QNetworkReply::NoError
                                 ? QString("HTTP %1").arg(statusCode)
                                 : reply->errorString();
        } else {
            cache_.store(key, body, statusCode, QDateTime::currentDateTime());
        }

        active_ = nullptr;
        reply->deleteLater();
        if (request.callback) {
            request.callback(response);
        }
        startNext();
    });
}
