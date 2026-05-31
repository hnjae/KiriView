// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
#include <QDir>
#include <QElapsedTimer>
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
    void visibleThumbnailClickDispatchesWithoutScrollMovement();
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

QTEST_MAIN(TestThumbnailPanel)

#include "test_thumbnailpanel.moc"
