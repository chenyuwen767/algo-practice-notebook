#include <QtTest>

#include <QCoreApplication>
#include <QDateTime>

#include "oj_client.h"

class OjClientTest : public QObject {
    Q_OBJECT

private slots:
    void cachesSuccessfulResponses();
    void expiresCacheAfterTtl();
    void ignoresNonSuccessResponses();
    void usesPlatformIntervals();
    void clearsCache();
};

void OjClientTest::cachesSuccessfulResponses() {
    OjCache cache;
    const QDateTime now = QDateTime::fromString("2026-10-31T10:00:00+08:00", Qt::ISODate);
    cache.store("key", QByteArray("body"), 200, now);

    OjResponse response;
    QVERIFY(cache.lookup("key", now.addSecs(299), response));
    QVERIFY(response.ok);
    QVERIFY(response.fromCache);
    QCOMPARE(response.httpStatus, 200);
    QCOMPARE(response.body, QByteArray("body"));
}

void OjClientTest::expiresCacheAfterTtl() {
    OjCache cache;
    const QDateTime now = QDateTime::fromString("2026-10-31T10:00:00+08:00", Qt::ISODate);
    cache.store("key", QByteArray("body"), 200, now);

    OjResponse response;
    QVERIFY(!cache.lookup("key", now.addSecs(OjCache::ttlSeconds + 1), response));
    QVERIFY(!response.ok);
}

void OjClientTest::ignoresNonSuccessResponses() {
    OjCache cache;
    const QDateTime now = QDateTime::fromString("2026-10-31T10:00:00+08:00", Qt::ISODate);
    cache.store("key", QByteArray("error"), 500, now);

    OjResponse response;
    QVERIFY(!cache.lookup("key", now, response));
}

void OjClientTest::usesPlatformIntervals() {
    QCOMPARE(OjClient::minimumIntervalMs(OjPlatform::Codeforces), 2000);
    QCOMPARE(OjClient::minimumIntervalMs(OjPlatform::Luogu), 1000);
}

void OjClientTest::clearsCache() {
    OjCache cache;
    const QDateTime now = QDateTime::fromString("2026-10-31T10:00:00+08:00", Qt::ISODate);
    cache.store("key", QByteArray("body"), 200, now);
    cache.clear();

    OjResponse response;
    QVERIFY(!cache.lookup("key", now, response));
}

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    OjClientTest instance;
    return QTest::qExec(&instance, argc, argv);
}

#include "test_oj_client.moc"
