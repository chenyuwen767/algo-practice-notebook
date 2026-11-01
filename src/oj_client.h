#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QString>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

enum class OjPlatform {
    Codeforces,
    Luogu
};

struct OjResponse {
    bool ok = false;
    bool fromCache = false;
    int httpStatus = 0;
    QByteArray body;
    QString error;
};

class OjCache {
public:
    static constexpr int ttlSeconds = 300;

    bool lookup(const QString& key, const QDateTime& now, OjResponse& response) const;
    void store(const QString& key, const QByteArray& body, int status, const QDateTime& now);
    void clear();

private:
    struct Entry {
        QByteArray body;
        int status = 0;
        QDateTime storedAt;
    };
    QHash<QString, Entry> entries_;
};

class OjClient : public QObject {
public:
    using Callback = std::function<void(const OjResponse&)>;

    explicit OjClient(QNetworkAccessManager* network, QObject* parent = nullptr);
    ~OjClient() override;

    void get(OjPlatform platform, const QUrl& url, bool forceRefresh, Callback callback);

    static int minimumIntervalMs(OjPlatform platform);

private:
    struct Request {
        OjPlatform platform = OjPlatform::Codeforces;
        QUrl url;
        int retryCount = 0;
        Callback callback;
    };

    void startNext();

    QNetworkAccessManager* network_;
    OjCache cache_;
    QQueue<Request> queue_;
    QPointer<QNetworkReply> active_;
    QHash<int, qint64> lastStartedMs_;
    bool waiting_ = false;
};
