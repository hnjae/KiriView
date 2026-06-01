// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickView>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtQml/qqml.h>
#include <algorithm>
#include <cmath>
#include <memory>

class TestThumbnailPanel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void visibleMainNavigationKeepsScrollPosition();
    void offscreenMainNavigationRevealsSelectedThumbnail();
    void adjacentMainNavigationRevealsSelectedThumbnail();
    void adjacentMainNavigationInsideSafeZoneKeepsScrollPosition();
    void adjacentMainNavigationFollowsTrailingSafeZone();
    void adjacentMainNavigationFollowsLeadingSafeZone();
    void resizedPanelContainsSelectedThumbnail();
    void adjacentNextNavigationUsesPreferredZoneLeadingAnchor();
    void adjacentPreviousNavigationUsesPreferredZoneTrailingAnchor();
    void largeJumpUsesContainmentAfterAdjacentDirection();
    void rapidAdjacentNavigationUsesLatestRevealTarget();
    void userThumbnailBrowsingSuppressesAdjacentPreferredZoneFollow();
    void userThumbnailBrowsingStillContainsLostCurrentThumbnail();
    void largeJumpNavigationRevealsSelectedThumbnail();
    void visibleThumbnailClickDispatchesWithoutScrollMovement();
    void scrolledThumbnailClickDispatchesWithoutScrollMovement();
    void delegateReportsSourceNeutralDemandAndKeepsFallbackIcon();
};

namespace {
constexpr int testImageCount = 18;
constexpr int panelWidth = 360;
constexpr int panelHeight = 96;
constexpr double fuzzyPixel = 0.75;

struct ThumbnailPanelFixture {
    std::unique_ptr<QQmlComponent> component;
    std::unique_ptr<QQuickView> view;
    std::unique_ptr<QTemporaryDir> directory;
    KiriDocumentSession *documentSession = nullptr;
    QQuickItem *root = nullptr;
    QQuickItem *thumbnailStrip = nullptr;
    QString errorString;

    bool isValid() const
    {
        return view != nullptr && root != nullptr && thumbnailStrip != nullptr
            && documentSession != nullptr;
    }
};

void addEnvironmentImportPaths(QQmlEngine &engine)
{
    const QString paths = qEnvironmentVariable("NIXPKGS_QML_SEARCH_PATHS");
    for (const QString &path : paths.split(QLatin1Char(':'), Qt::SkipEmptyParts)) {
        engine.addImportPath(path);
    }
}

void registerKiriViewQmlTypes()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    KiriView::initializeLocalization();
    qmlRegisterType<KiriDocumentSession>("io.github.hnjae.kiriview", 1, 0, "KiriDocumentSession");
    registered = true;
}

QString qmlSourceImport()
{
    const QString qmlPath = QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
                                .absoluteFilePath(QStringLiteral("../../src/qml"));
    return QUrl::fromLocalFile(qmlPath).toString();
}

QString fixtureQml(const QString &sourceUrl)
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml

Item {
    id: root

    width: %2
    height: %3

    KiriDocumentSession {
        id: documentSession
        objectName: "documentSession"
        sourceUrl: "%4"
    }

    KiriViewQml.ThumbnailPanel {
        id: thumbnailPanel

        anchors.fill: parent
        documentSession: documentSession
        viewerForegroundColor: "white"
        viewerSurfaceColor: "black"
    }
}
)")
        .arg(qmlSourceImport())
        .arg(panelWidth)
        .arg(panelHeight)
        .arg(sourceUrl);
}

bool writeTestImages(QTemporaryDir &directory, QString *firstImagePath, QString *errorString)
{
    QImage image(QSize(3, 2), QImage::Format_RGBA8888);
    image.fill(Qt::blue);

    for (int number = 1; number <= testImageCount; ++number) {
        const QString path
            = directory.filePath(QStringLiteral("%1.png").arg(number, 2, 10, QLatin1Char('0')));
        if (!image.save(path, "PNG")) {
            *errorString = QStringLiteral("failed to save test image %1").arg(path);
            return false;
        }
        if (number == 1) {
            *firstImagePath = path;
        }
    }

    return true;
}

ThumbnailPanelFixture createFixture()
{
    ThumbnailPanelFixture fixture;
    registerKiriViewQmlTypes();
    fixture.directory = std::make_unique<QTemporaryDir>();
    if (!fixture.directory->isValid()) {
        fixture.errorString = QStringLiteral("temporary image directory was not created");
        return fixture;
    }

    QString firstImagePath;
    if (!writeTestImages(*fixture.directory, &firstImagePath, &fixture.errorString)) {
        return fixture;
    }

    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(panelWidth, panelHeight);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());
    fixture.view->engine()->addImportPath(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml")));
    KLocalization::setupLocalizedContext(fixture.view->engine());

    fixture.component = std::make_unique<QQmlComponent>(fixture.view->engine());
    fixture.component->setData(fixtureQml(QUrl::fromLocalFile(firstImagePath).toString()).toUtf8(),
        QUrl(QStringLiteral("memory:test_thumbnailpanel.qml")));
    for (int attempt = 0; fixture.component->isLoading() && attempt < 100; ++attempt) {
        QCoreApplication::processEvents();
        QTest::qWait(10);
    }
    if (fixture.component->isLoading()) {
        fixture.errorString = QStringLiteral("QML component did not become ready");
        return fixture;
    }
    if (fixture.component->isError()) {
        fixture.errorString = fixture.component->errorString();
        return fixture;
    }

    QObject *createdRoot = fixture.component->create();
    if (createdRoot == nullptr) {
        fixture.errorString = fixture.component->errorString();
        return fixture;
    }

    fixture.root = qobject_cast<QQuickItem *>(createdRoot);
    if (fixture.root == nullptr) {
        fixture.errorString = QStringLiteral("fixture root is not a QQuickItem");
        delete createdRoot;
        return fixture;
    }

    fixture.view->setContent(QUrl(QStringLiteral("memory:test_thumbnailpanel.qml")),
        fixture.component.get(), createdRoot);
    fixture.view->show();
    QCoreApplication::processEvents();

    fixture.documentSession
        = fixture.root->findChild<KiriDocumentSession *>(QStringLiteral("documentSession"));
    fixture.thumbnailStrip
        = fixture.root->findChild<QQuickItem *>(QStringLiteral("thumbnailStrip"));
    if (!fixture.isValid()) {
        fixture.errorString = QStringLiteral("fixture did not create required objects");
    }
    return fixture;
}

double realProperty(const QObject &object, const char *name)
{
    return object.property(name).toDouble();
}

double itemStart(const QQuickItem &thumbnailStrip, int number)
{
    return (number - 1) * realProperty(thumbnailStrip, "itemPitch");
}

double contentX(const QQuickItem &thumbnailStrip)
{
    return realProperty(thumbnailStrip, "contentX");
}

bool nearlyEqual(double left, double right) { return std::abs(left - right) <= fuzzyPixel; }

double clampedContentXForSnapPosition(const QQuickItem &thumbnailStrip, int number, double snap)
{
    const double contentWidth = realProperty(thumbnailStrip, "contentWidth");
    const double maxContentX = std::max(0.0, contentWidth - thumbnailStrip.width());
    return std::max(0.0, std::min(maxContentX, itemStart(thumbnailStrip, number) - snap));
}

double clampedContentXForContainment(const QQuickItem &thumbnailStrip, int number)
{
    const double viewportStart = contentX(thumbnailStrip);
    const double viewportEnd = viewportStart + thumbnailStrip.width();
    const double itemBegin = itemStart(thumbnailStrip, number);
    const double itemEnd = itemBegin + realProperty(thumbnailStrip, "delegateWidth");
    double target = viewportStart;
    if (itemBegin < viewportStart) {
        target = itemBegin;
    } else if (itemEnd > viewportEnd) {
        target = itemEnd - thumbnailStrip.width();
    }

    const double contentWidth = realProperty(thumbnailStrip, "contentWidth");
    const double maxContentX = std::max(0.0, contentWidth - thumbnailStrip.width());
    return std::max(0.0, std::min(maxContentX, target));
}

double currentThumbnailStartInViewport(const QQuickItem &thumbnailStrip)
{
    const int currentIndex = thumbnailStrip.property("currentIndex").toInt();
    return currentIndex * realProperty(thumbnailStrip, "itemPitch") - contentX(thumbnailStrip);
}

bool waitForActiveNavigation(const KiriDocumentSession &documentSession, int current, int count)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 10000) {
        if (documentSession.activeNavigationKnown()
            && documentSession.activeNavigationCurrentNumber() == current
            && documentSession.activeNavigationCount() == count) {
            return true;
        }
        QCoreApplication::processEvents();
        QTest::qWait(10);
    }

    return false;
}

bool waitForContentX(const QQuickItem &thumbnailStrip, double expected)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 1000) {
        if (nearlyEqual(contentX(thumbnailStrip), expected)) {
            return true;
        }
        QCoreApplication::processEvents();
        QTest::qWait(10);
    }

    return false;
}

void setContentX(QQuickItem &thumbnailStrip, double value)
{
    thumbnailStrip.setProperty("contentX", value);
    QCoreApplication::processEvents();
}

bool currentThumbnailFullyVisible(const QQuickItem &thumbnailStrip)
{
    const int currentIndex = thumbnailStrip.property("currentIndex").toInt();
    if (currentIndex < 0) {
        return false;
    }

    const double start = currentIndex * realProperty(thumbnailStrip, "itemPitch");
    const double end = start + realProperty(thumbnailStrip, "delegateWidth");
    const double viewportStart = contentX(thumbnailStrip);
    const double viewportEnd = viewportStart + thumbnailStrip.width();
    return start >= viewportStart - fuzzyPixel && end <= viewportEnd + fuzzyPixel;
}

QQuickItem *thumbnailDelegateForNumber(QQuickItem &item, int number)
{
    if (item.objectName() == QStringLiteral("thumbnailStripItem")
        && item.property("number").toInt() == number) {
        return &item;
    }

    for (QQuickItem *child : item.childItems()) {
        if (QQuickItem *delegate = thumbnailDelegateForNumber(*child, number);
            delegate != nullptr) {
            return delegate;
        }
    }

    return nullptr;
}

void noteUserThumbnailScroll(QQuickItem &thumbnailStrip)
{
    QVERIFY(QMetaObject::invokeMethod(
        &thumbnailStrip, "noteUserThumbnailScroll", Qt::DirectConnection));
}
}

void TestThumbnailPanel::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}

void TestThumbnailPanel::init()
{
    QTest::failOnWarning(QRegularExpression(
        QStringLiteral(".*Created graphical object was not placed in the graphics scene.*")));
}

void TestThumbnailPanel::delegateReportsSourceNeutralDemandAndKeepsFallbackIcon()
{
    QFile thumbnailPanelFile(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml/ThumbnailPanel.qml")));
    QVERIFY(thumbnailPanelFile.open(QIODevice::ReadOnly | QIODevice::Text));

    const QString source = QString::fromUtf8(thumbnailPanelFile.readAll());
    QVERIFY(source.contains(QStringLiteral("objectName: \"thumbnailPreviewBox\"")));
    QVERIFY(source.contains(QStringLiteral("effectiveDevicePixelRatio")));
    QVERIFY(source.contains(QStringLiteral("activeNavigationThumbnailDemandBucket")));
    QVERIFY(source.contains(QStringLiteral("reportActiveNavigationThumbnailDemand")));
    QVERIFY(source.contains(QStringLiteral("navigationGeneration")));
    QVERIFY(source.contains(QStringLiteral("required property int thumbnailStatus")));
    QVERIFY(source.contains(QStringLiteral("required property url thumbnailImageSource")));
    QVERIFY(source.contains(QStringLiteral("KiriDocumentSession.ReadyThumbnailResult")));
    QVERIFY(source.contains(QStringLiteral("source: thumbnailDelegate.thumbnailImageSource")));
    QVERIFY(source.contains(QStringLiteral("Kirigami.Icon")));
    QVERIFY(source.contains(QStringLiteral("source: thumbnailDelegate.iconName")));
    QVERIFY(!source.contains(QStringLiteral("DirectImage")));
    QVERIFY(!source.contains(QStringLiteral("DirectVideo")));
    QVERIFY(!source.contains(QStringLiteral("ImageDocumentPage")));
}

void TestThumbnailPanel::visibleMainNavigationKeepsScrollPosition()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    const double visibleScrollPosition = itemStart(*fixture.thumbnailStrip, 5);
    setContentX(*fixture.thumbnailStrip, visibleScrollPosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, visibleScrollPosition),
        "thumbnail strip did not accept the initial scroll position");

    fixture.documentSession->openActiveNavigationAtNumber(7);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 7, testImageCount),
        "main-view navigation did not select the requested item");
    QTest::qWait(180);

    QVERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY2(nearlyEqual(contentX(*fixture.thumbnailStrip), visibleScrollPosition),
        qPrintable(QStringLiteral("contentX moved from %1 to %2")
                .arg(visibleScrollPosition)
                .arg(contentX(*fixture.thumbnailStrip))));
}

void TestThumbnailPanel::offscreenMainNavigationRevealsSelectedThumbnail()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    setContentX(*fixture.thumbnailStrip, 0.0);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, 0.0),
        "thumbnail strip did not return to the leading edge");

    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 2, testImageCount),
        "main-view navigation did not select the adjacent item");
    QTest::qWait(180);

    QVERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY2(nearlyEqual(contentX(*fixture.thumbnailStrip), 0.0),
        qPrintable(QStringLiteral("contentX moved to %1").arg(contentX(*fixture.thumbnailStrip))));
}

void TestThumbnailPanel::adjacentMainNavigationRevealsSelectedThumbnail()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(10);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 10, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    setContentX(*fixture.thumbnailStrip, 0.0);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, 0.0),
        "thumbnail strip did not return to the leading edge");

    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 11, testImageCount),
        "adjacent navigation did not select the offscreen item");

    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY(contentX(*fixture.thumbnailStrip) > 0.0);
}

void TestThumbnailPanel::adjacentMainNavigationInsideSafeZoneKeepsScrollPosition()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(4);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 4, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    const double stablePosition = itemStart(*fixture.thumbnailStrip, 4);
    setContentX(*fixture.thumbnailStrip, stablePosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, stablePosition),
        "thumbnail strip did not accept the setup scroll position");

    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 5, testImageCount),
        "adjacent navigation did not select the next item");
    QTest::qWait(180);

    QVERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY2(nearlyEqual(contentX(*fixture.thumbnailStrip), stablePosition),
        qPrintable(QStringLiteral("contentX moved from %1 to %2")
                .arg(stablePosition)
                .arg(contentX(*fixture.thumbnailStrip))));
}

void TestThumbnailPanel::adjacentMainNavigationFollowsTrailingSafeZone()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(5);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 5, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    const double trailingEdgePosition = itemStart(*fixture.thumbnailStrip, 4);
    setContentX(*fixture.thumbnailStrip, trailingEdgePosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, trailingEdgePosition),
        "thumbnail strip did not accept the setup scroll position");

    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 6, testImageCount),
        "adjacent navigation did not select the next item");

    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QTRY_VERIFY2(contentX(*fixture.thumbnailStrip) > trailingEdgePosition + fuzzyPixel,
        qPrintable(QStringLiteral("contentX stayed at %1 from %2")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(trailingEdgePosition)));
}

void TestThumbnailPanel::adjacentMainNavigationFollowsLeadingSafeZone()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(6);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 6, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    const double leadingEdgePosition = itemStart(*fixture.thumbnailStrip, 5);
    setContentX(*fixture.thumbnailStrip, leadingEdgePosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, leadingEdgePosition),
        "thumbnail strip did not accept the setup scroll position");

    fixture.documentSession->openPreviousActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 5, testImageCount),
        "adjacent navigation did not select the previous item");

    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QTRY_VERIFY2(contentX(*fixture.thumbnailStrip) < leadingEdgePosition - fuzzyPixel,
        qPrintable(QStringLiteral("contentX stayed at %1 from %2")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(leadingEdgePosition)));
}

void TestThumbnailPanel::resizedPanelContainsSelectedThumbnail()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(9);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 9, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    const double widePanelPosition = itemStart(*fixture.thumbnailStrip, 7);
    setContentX(*fixture.thumbnailStrip, widePanelPosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, widePanelPosition),
        "thumbnail strip did not accept the setup scroll position");
    QVERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    fixture.view->resize(panelWidth / 2, panelHeight);
    QCoreApplication::processEvents();

    QTRY_VERIFY(fixture.thumbnailStrip->width() < panelWidth);
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY2(contentX(*fixture.thumbnailStrip) > widePanelPosition + fuzzyPixel,
        qPrintable(QStringLiteral("contentX stayed at %1 from %2 after resize")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(widePanelPosition)));
}

void TestThumbnailPanel::adjacentNextNavigationUsesPreferredZoneLeadingAnchor()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(10);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 10, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    setContentX(*fixture.thumbnailStrip, 0.0);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, 0.0),
        "thumbnail strip did not return to the leading edge");

    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 11, testImageCount),
        "adjacent navigation did not select the next item");

    const double expected = clampedContentXForSnapPosition(
        *fixture.thumbnailStrip, 11, realProperty(*fixture.thumbnailStrip, "preferredZoneStart"));
    QTRY_VERIFY2(waitForContentX(*fixture.thumbnailStrip, expected),
        qPrintable(QStringLiteral("contentX is %1, expected %2")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(expected)));
    QVERIFY2(nearlyEqual(currentThumbnailStartInViewport(*fixture.thumbnailStrip),
                 realProperty(*fixture.thumbnailStrip, "preferredZoneStart")),
        qPrintable(QStringLiteral("selected thumbnail start is %1, preferred start is %2")
                .arg(currentThumbnailStartInViewport(*fixture.thumbnailStrip))
                .arg(realProperty(*fixture.thumbnailStrip, "preferredZoneStart"))));
}

void TestThumbnailPanel::adjacentPreviousNavigationUsesPreferredZoneTrailingAnchor()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(10);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 10, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    const double setupPosition = itemStart(*fixture.thumbnailStrip, 10);
    setContentX(*fixture.thumbnailStrip, setupPosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, setupPosition),
        "thumbnail strip did not accept the setup scroll position");

    fixture.documentSession->openPreviousActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 9, testImageCount),
        "adjacent navigation did not select the previous item");

    const double trailingSnap = realProperty(*fixture.thumbnailStrip, "preferredZoneEnd")
        - realProperty(*fixture.thumbnailStrip, "delegateWidth");
    const double expected
        = clampedContentXForSnapPosition(*fixture.thumbnailStrip, 9, trailingSnap);
    QTRY_VERIFY2(waitForContentX(*fixture.thumbnailStrip, expected),
        qPrintable(QStringLiteral("contentX is %1, expected %2")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(expected)));
    QVERIFY2(nearlyEqual(currentThumbnailStartInViewport(*fixture.thumbnailStrip), trailingSnap),
        qPrintable(QStringLiteral("selected thumbnail start is %1, trailing snap is %2")
                .arg(currentThumbnailStartInViewport(*fixture.thumbnailStrip))
                .arg(trailingSnap)));
}

void TestThumbnailPanel::largeJumpUsesContainmentAfterAdjacentDirection()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(5);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 5, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 6, testImageCount),
        "adjacent navigation did not select the next item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    setContentX(*fixture.thumbnailStrip, 0.0);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, 0.0),
        "thumbnail strip did not return to the leading edge");

    const double expected = clampedContentXForContainment(*fixture.thumbnailStrip, 16);
    const double staleDirectionTarget = clampedContentXForSnapPosition(
        *fixture.thumbnailStrip, 16, realProperty(*fixture.thumbnailStrip, "preferredZoneStart"));
    QVERIFY(std::abs(expected - staleDirectionTarget) > fuzzyPixel);

    fixture.documentSession->openActiveNavigationAtNumber(16);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 16, testImageCount),
        "large jump did not select the requested item");

    QTRY_VERIFY2(waitForContentX(*fixture.thumbnailStrip, expected),
        qPrintable(QStringLiteral("contentX is %1, expected containment target %2")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(expected)));
}

void TestThumbnailPanel::rapidAdjacentNavigationUsesLatestRevealTarget()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(5);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 5, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    const double setupPosition = itemStart(*fixture.thumbnailStrip, 4);
    setContentX(*fixture.thumbnailStrip, setupPosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, setupPosition),
        "thumbnail strip did not accept the setup scroll position");

    QTest::qWait(220);
    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 6, testImageCount),
        "first adjacent navigation did not select the next item");
    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 7, testImageCount),
        "second adjacent navigation did not select the latest item");

    const double expected = clampedContentXForSnapPosition(
        *fixture.thumbnailStrip, 7, realProperty(*fixture.thumbnailStrip, "preferredZoneStart"));
    QTRY_VERIFY2(waitForContentX(*fixture.thumbnailStrip, expected),
        qPrintable(QStringLiteral("contentX is %1, expected latest target %2")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(expected)));
    QVERIFY2(nearlyEqual(currentThumbnailStartInViewport(*fixture.thumbnailStrip),
                 realProperty(*fixture.thumbnailStrip, "preferredZoneStart")),
        qPrintable(QStringLiteral("selected thumbnail start is %1, preferred start is %2")
                .arg(currentThumbnailStartInViewport(*fixture.thumbnailStrip))
                .arg(realProperty(*fixture.thumbnailStrip, "preferredZoneStart"))));
}

void TestThumbnailPanel::userThumbnailBrowsingSuppressesAdjacentPreferredZoneFollow()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(5);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 5, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    const double userBrowsePosition = itemStart(*fixture.thumbnailStrip, 4);
    setContentX(*fixture.thumbnailStrip, userBrowsePosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, userBrowsePosition),
        "thumbnail strip did not accept the user scroll position");
    noteUserThumbnailScroll(*fixture.thumbnailStrip);

    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 6, testImageCount),
        "adjacent navigation did not select the next item");
    QTest::qWait(220);

    QVERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY2(nearlyEqual(contentX(*fixture.thumbnailStrip), userBrowsePosition),
        qPrintable(QStringLiteral("contentX moved from %1 to %2")
                .arg(userBrowsePosition)
                .arg(contentX(*fixture.thumbnailStrip))));
}

void TestThumbnailPanel::userThumbnailBrowsingStillContainsLostCurrentThumbnail()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    fixture.documentSession->openActiveNavigationAtNumber(10);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 10, testImageCount),
        "large jump did not select the setup item");
    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));

    setContentX(*fixture.thumbnailStrip, 0.0);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, 0.0),
        "thumbnail strip did not return to the leading edge");
    noteUserThumbnailScroll(*fixture.thumbnailStrip);

    const double expected = clampedContentXForContainment(*fixture.thumbnailStrip, 11);
    fixture.documentSession->openNextActiveNavigation();
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 11, testImageCount),
        "adjacent navigation did not select the next item");

    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QTRY_VERIFY2(waitForContentX(*fixture.thumbnailStrip, expected),
        qPrintable(QStringLiteral("contentX is %1, expected containment target %2")
                .arg(contentX(*fixture.thumbnailStrip))
                .arg(expected)));
}

void TestThumbnailPanel::largeJumpNavigationRevealsSelectedThumbnail()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    setContentX(*fixture.thumbnailStrip, 0.0);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, 0.0),
        "thumbnail strip did not return to the leading edge");

    fixture.documentSession->openActiveNavigationAtNumber(16);
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 16, testImageCount),
        "main-view navigation did not select the offscreen item");

    QTRY_VERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY(contentX(*fixture.thumbnailStrip) > 0.0);
}

void TestThumbnailPanel::visibleThumbnailClickDispatchesWithoutScrollMovement()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    const int clickedNumber = 6;
    const double visibleScrollPosition = itemStart(*fixture.thumbnailStrip, 4);
    setContentX(*fixture.thumbnailStrip, visibleScrollPosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, visibleScrollPosition),
        "thumbnail strip did not accept the initial scroll position");

    QQuickItem *delegate = nullptr;
    QTRY_VERIFY(
        (delegate = thumbnailDelegateForNumber(*fixture.thumbnailStrip, clickedNumber)) != nullptr);
    const QPointF clickPosition
        = delegate->mapToScene(QPointF(delegate->width() / 2.0, delegate->height() / 2.0));
    QTest::mouseClick(fixture.view.get(), Qt::LeftButton, Qt::NoModifier, clickPosition.toPoint());

    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, clickedNumber, testImageCount),
        "thumbnail click did not dispatch selection");
    QTest::qWait(180);

    QVERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY2(nearlyEqual(contentX(*fixture.thumbnailStrip), visibleScrollPosition),
        qPrintable(QStringLiteral("contentX moved from %1 to %2")
                .arg(visibleScrollPosition)
                .arg(contentX(*fixture.thumbnailStrip))));
}

void TestThumbnailPanel::scrolledThumbnailClickDispatchesWithoutScrollMovement()
{
    ThumbnailPanelFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, 1, testImageCount),
        "active navigation did not become ready");

    const int clickedNumber = 16;
    const double scrolledPosition = itemStart(*fixture.thumbnailStrip, 14);
    setContentX(*fixture.thumbnailStrip, scrolledPosition);
    QVERIFY2(waitForContentX(*fixture.thumbnailStrip, scrolledPosition),
        "thumbnail strip did not accept the user scroll position");

    QQuickItem *delegate = nullptr;
    QTRY_VERIFY(
        (delegate = thumbnailDelegateForNumber(*fixture.thumbnailStrip, clickedNumber)) != nullptr);
    const QPointF clickPosition
        = delegate->mapToScene(QPointF(delegate->width() / 2.0, delegate->height() / 2.0));
    QTest::mouseClick(fixture.view.get(), Qt::LeftButton, Qt::NoModifier, clickPosition.toPoint());

    QVERIFY2(waitForActiveNavigation(*fixture.documentSession, clickedNumber, testImageCount),
        "thumbnail click did not dispatch selection");
    QTest::qWait(180);

    QVERIFY(currentThumbnailFullyVisible(*fixture.thumbnailStrip));
    QVERIFY2(nearlyEqual(contentX(*fixture.thumbnailStrip), scrolledPosition),
        qPrintable(QStringLiteral("contentX moved from %1 to %2")
                .arg(scrolledPosition)
                .arg(contentX(*fixture.thumbnailStrip))));
}

QTEST_MAIN(TestThumbnailPanel)

#include "test_thumbnailpanel.moc"
