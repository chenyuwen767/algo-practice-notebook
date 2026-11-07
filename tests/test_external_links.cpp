#include <QtTest>

#include <QString>

#include "external_links.h"

class ExternalLinksTest : public QObject {
    Q_OBJECT

private slots:
    void mapsEdgeProgId();
    void mapsChromiumBasedProgIds();
    void mapsFirefoxAndOpera();
    void ignoresUnknownProgId();
};

void ExternalLinksTest::mapsEdgeProgId() {
    QCOMPARE(taskui::browserExecutableName("MSEdgeHTM"), QString("msedge.exe"));
    QCOMPARE(taskui::browserExecutableName("MSEdgeDHTML"), QString("msedge.exe"));
    QCOMPARE(taskui::browserExecutableName("msedgehtm"), QString("msedge.exe"));
}

void ExternalLinksTest::mapsChromiumBasedProgIds() {
    QCOMPARE(taskui::browserExecutableName("ChromeHTML"), QString("chrome.exe"));
    QCOMPARE(taskui::browserExecutableName("BraveHTML"), QString("brave.exe"));
}

void ExternalLinksTest::mapsFirefoxAndOpera() {
    QCOMPARE(taskui::browserExecutableName("FirefoxURL-308046B0AF4A39CB"), QString("firefox.exe"));
    QCOMPARE(taskui::browserExecutableName("OperaStable"), QString("opera.exe"));
}

void ExternalLinksTest::ignoresUnknownProgId() {
    QVERIFY(taskui::browserExecutableName(QString()).isEmpty());
    QVERIFY(taskui::browserExecutableName("SomeOtherBrowser.Url").isEmpty());
}

QTEST_MAIN(ExternalLinksTest)
#include "test_external_links.moc"
