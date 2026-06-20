// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QSignalSpy>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <memory>

class TestDisplayImagePage : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bindsProjectionToProviderImage();
    void transposesProviderImageGeometryForSidewaysRotation();
    void acknowledgesLoadedProviderRevision();
    void skipsAcknowledgmentWhenProjectionDoesNotRequireIt();
    void reportsMissingAcknowledgmentForRequiredEmptyProviderUrl();
};

namespace {
class SolidImageProvider final : public QQuickImageProvider
{
public:
    SolidImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    QImage requestImage(const QString &, QSize *size, const QSize &requestedSize) override
    {
        const QSize imageSize
            = requestedSize.isValid() && !requestedSize.isEmpty() ? requestedSize : QSize(12, 8);
        if (size != nullptr) {
            *size = imageSize;
        }

        QImage image(imageSize, QImage::Format_RGBA8888);
        image.fill(Qt::white);
        return image;
    }
};

struct Fixture {
    std::unique_ptr<QQmlEngine> engine;
    QObject *root = nullptr;
    QString errorString;

    bool isValid() const { return engine != nullptr && root != nullptr; }
};

QString qmlSourceImport()
{
    const QString qmlPath = QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
                                .absoluteFilePath(QStringLiteral("../../src/qml"));
    return QUrl::fromLocalFile(qmlPath).toString();
}

QString fixtureQml(bool loadAcknowledgmentRequired, const QString &providerUrl)
{
    return QStringLiteral(R"(
import QtQuick
import "%1" as KiriViewQml

Item {
    id: root

    width: 80
    height: 60
    property alias page: page
    property alias displaySource: displaySource

    QtObject {
        id: displaySource

        property bool visible: true
        property url providerUrl: "%2"
        property string revisionToken: "7"
        property string sourceIdentity: "source-a"
        property size sourceSizeHint: Qt.size(32, 0)
        property bool cacheEnabled: false
        property bool loadAcknowledgmentRequired: %3
        property int rotationDegrees: 90
        property bool retainWhileLoadingEligible: false
    }

    KiriViewQml.DisplayImagePage {
        id: page

        objectName: "displayImagePage"
        anchors.fill: parent
        displaySource: displaySource
    }
}
)")
        .arg(qmlSourceImport(), providerUrl,
            loadAcknowledgmentRequired ? QStringLiteral("true") : QStringLiteral("false"));
}

Fixture createFixture(bool loadAcknowledgmentRequired = true,
    const QString &providerUrl = QStringLiteral("image://test-display/ready"))
{
    Fixture fixture;
    fixture.engine = std::make_unique<QQmlEngine>();
    fixture.engine->addImageProvider(QStringLiteral("test-display"), new SolidImageProvider);

    QQmlComponent component(fixture.engine.get());
    component.setData(fixtureQml(loadAcknowledgmentRequired, providerUrl).toUtf8(),
        QUrl(QStringLiteral("memory:test_displayimagepage.qml")));
    for (int attempt = 0; component.isLoading() && attempt < 100; ++attempt) {
        QCoreApplication::processEvents();
        QTest::qWait(10);
    }
    if (component.isLoading()) {
        fixture.errorString = QStringLiteral("QML component did not become ready");
        return fixture;
    }
    if (component.isError()) {
        fixture.errorString = component.errorString();
        return fixture;
    }

    fixture.root = component.create();
    if (fixture.root == nullptr) {
        fixture.errorString = component.errorString();
        return fixture;
    }

    return fixture;
}

QObject *displayImagePage(const Fixture &fixture)
{
    return fixture.root == nullptr
        ? nullptr
        : fixture.root->findChild<QObject *>(QStringLiteral("displayImagePage"));
}

QObject *providerImage(const Fixture &fixture)
{
    return fixture.root == nullptr
        ? nullptr
        : fixture.root->findChild<QObject *>(QStringLiteral("providerImage"));
}
}

void TestDisplayImagePage::bindsProjectionToProviderImage()
{
    Fixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QObject *image = providerImage(fixture);
    QVERIFY(image != nullptr);
    QTRY_COMPARE(image->property("status").toInt(), 1);
    QCOMPARE(image->property("source").toUrl(), QUrl(QStringLiteral("image://test-display/ready")));
    QCOMPARE(image->property("sourceSize").toSize(), QSize(32, 0));
    QCOMPARE(image->property("cache").toBool(), false);
    QCOMPARE(image->property("asynchronous").toBool(), false);
    QCOMPARE(image->property("retainWhileLoading").toBool(), false);
    QCOMPARE(image->property("rotation").toReal(), 90.0);
    QCOMPARE(image->property("smooth").toBool(), true);
    QCOMPARE(image->property("mipmap").toBool(), true);
}

void TestDisplayImagePage::transposesProviderImageGeometryForSidewaysRotation()
{
    Fixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QObject *image = providerImage(fixture);
    QVERIFY(image != nullptr);

    QCOMPARE(image->property("width").toReal(), 60.0);
    QCOMPARE(image->property("height").toReal(), 80.0);
    QCOMPARE(image->property("x").toReal(), 10.0);
    QCOMPARE(image->property("y").toReal(), -10.0);
}

void TestDisplayImagePage::acknowledgesLoadedProviderRevision()
{
    Fixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QObject *page = displayImagePage(fixture);
    QVERIFY(page != nullptr);
    QSignalSpy spy(page, SIGNAL(loadOutcomeAcknowledged(QUrl, QString, QString, int)));
    QVERIFY(spy.isValid());

    QTRY_COMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toUrl(), QUrl(QStringLiteral("image://test-display/ready")));
    QCOMPARE(arguments.at(1).toString(), QStringLiteral("7"));
    QCOMPARE(arguments.at(2).toString(), QStringLiteral("source-a"));
    QCOMPARE(arguments.at(3).toInt(), 0);
}

void TestDisplayImagePage::skipsAcknowledgmentWhenProjectionDoesNotRequireIt()
{
    Fixture fixture = createFixture(false);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QObject *page = displayImagePage(fixture);
    QVERIFY(page != nullptr);
    QSignalSpy spy(page, SIGNAL(loadOutcomeAcknowledged(QUrl, QString, QString, int)));
    QVERIFY(spy.isValid());

    QTest::qWait(100);
    QCOMPARE(spy.count(), 0);
}

void TestDisplayImagePage::reportsMissingAcknowledgmentForRequiredEmptyProviderUrl()
{
    Fixture fixture = createFixture(true, QString());
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QObject *page = displayImagePage(fixture);
    QVERIFY(page != nullptr);
    QSignalSpy spy(page, SIGNAL(loadOutcomeAcknowledged(QUrl, QString, QString, int)));
    QVERIFY(spy.isValid());

    QTRY_COMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toUrl(), QUrl());
    QCOMPARE(arguments.at(1).toString(), QStringLiteral("7"));
    QCOMPARE(arguments.at(2).toString(), QStringLiteral("source-a"));
    QCOMPARE(arguments.at(3).toInt(), 2);
}

QTEST_MAIN(TestDisplayImagePage)

#include "test_displayimagepage.moc"
