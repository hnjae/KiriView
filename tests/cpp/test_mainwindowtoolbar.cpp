// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationruntime.h"
#include "facade/imageactionavailability.h"
#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedocument.h"
#include "facade/kiriimageviewportcontextbridge.h"
#include "facade/kirimediainformation.h"
#include "facade/kirivideodocument.h"
#include "facade/kiriviewapplication.h"
#include "facade/menuaccesskeyrouter.h"
#include "kiriviewstate.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
#include <KZip>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QVariantMap>
#include <QWheelEvent>
#include <QtQml/qqml.h>
#include <cmath>
#include <memory>

class TestMainWindowToolBar : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void startupCreatesOneVisibleToolbarWithDisabledMediaControls();
    void startupInitialDirectImageRendersMainViewport();
    void startupInitialComicArchiveRendersAndNavigatesMainViewport();
    void directImageShowsMediaPositionAfterSiblingListing();
    void directoryImageDocumentShowsPagePosition();
    void mediaViewportHostLoadsOnlyActiveDelegate();
    void panelActionsToggleResizablePanels();
    void infoPanelUsesOverlayDrawerOnNarrowWindows();
    void escapeClosesInfoPanelBeforeLeavingFullscreen();
    void panelShortcutsToggleResizablePanels();
    void commandFixedShortcutsUseApplicationActions();
    void viewerRightClickOpensContextMenuOnlyFromMediaViewport();
    void toolbarZoomWheelAppliesFineManualStep();
    void rightButtonWheelSuppressesContextMenuTap();
    void fullscreenReusesSingleToolbarAndHidesApplicationMenuButton();
};

namespace {
struct MainWindowFixture {
    std::unique_ptr<QQmlApplicationEngine> engine;
    QQuickWindow *window = nullptr;
    QString errorString;

    bool isValid() const { return engine != nullptr && window != nullptr; }
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
    qmlRegisterType<KiriViewApplication>("io.github.hnjae.kiriview", 1, 0, "KiriViewApplication");
    qmlRegisterType<ImageActionAvailability>(
        "io.github.hnjae.kiriview", 1, 0, "ImageActionAvailability");
    qmlRegisterType<KiriDocumentSession>("io.github.hnjae.kiriview", 1, 0, "KiriDocumentSession");
    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<KiriImageViewportContextBridge>(
        "io.github.hnjae.kiriview", 1, 0, "KiriImageViewportContextBridge");
    qmlRegisterUncreatableType<KiriMediaInformation>("io.github.hnjae.kiriview", 1, 0,
        "KiriMediaInformation", "KiriMediaInformation is owned by KiriDocumentSession");
    qmlRegisterType<KiriVideoDocument>("io.github.hnjae.kiriview", 1, 0, "KiriVideoDocument");
    qmlRegisterType<MenuAccessKeyRouter>("io.github.hnjae.kiriview", 1, 0, "MenuAccessKeyRouter");
    registered = true;
}

void resetConfig()
{
    KiriViewState *state = KiriViewState::self();
    state->config()->deleteGroup(QStringLiteral("Interface"));
    state->config()->sync();
    state->config()->reparseConfiguration();
    state->read();
}

QUrl mainQmlUrl()
{
    return QUrl::fromLocalFile(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml/Main.qml")));
}

QList<QQuickItem *> controlToolBars(QObject *root)
{
    QList<QQuickItem *> toolbars;
    const QList<QQuickItem *> items
        = root->findChildren<QQuickItem *>(QString(), Qt::FindChildrenRecursively);
    for (QQuickItem *item : items) {
        if (item->inherits("QQuickToolBar")) {
            toolbars.append(item);
        }
    }
    return toolbars;
}

bool effectivelyVisible(QQuickItem *item)
{
    for (QQuickItem *current = item; current != nullptr; current = current->parentItem()) {
        if (!current->isVisible()) {
            return false;
        }
    }

    return item != nullptr;
}

QList<QQuickItem *> visibleItemsByObjectName(QObject *root, const QString &objectName)
{
    QList<QQuickItem *> visibleItems;
    const QList<QQuickItem *> items
        = root->findChildren<QQuickItem *>(objectName, Qt::FindChildrenRecursively);
    for (QQuickItem *item : items) {
        if (effectivelyVisible(item)) {
            visibleItems.append(item);
        }
    }
    return visibleItems;
}

QQuickItem *findQuickItem(QObject *root, const QString &objectName)
{
    return root->findChild<QQuickItem *>(objectName, Qt::FindChildrenRecursively);
}

QObject *findObject(QObject *root, const QString &objectName)
{
    return root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
}

KiriDocumentSession *findDocumentSession(QObject *root)
{
    return root->findChild<KiriDocumentSession *>(
        QStringLiteral("documentSession"), Qt::FindChildrenRecursively);
}

KiriViewApplication *findApplication(QObject *root)
{
    return root->findChild<KiriViewApplication *>(QString(), Qt::FindChildrenRecursively);
}

bool writeTestPng(const QString &path)
{
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    return image.save(path, "PNG");
}

QByteArray encodedTestPng(Qt::GlobalColor color)
{
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(color);

    QByteArray data;
    QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        return {};
    }

    return data;
}

bool writeEmptyFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly);
}

std::unique_ptr<QTemporaryDir> createMediaDirectory(
    QString *imageSourcePath, QString *videoSourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary media directory was not created");
        return nullptr;
    }

    const QString firstImage = directory->filePath(QStringLiteral("01.png"));
    const QString secondVideo = directory->filePath(QStringLiteral("02.mp4"));
    const QString thirdImage = directory->filePath(QStringLiteral("03.png"));
    if (!writeTestPng(firstImage) || !writeEmptyFile(secondVideo) || !writeTestPng(thirdImage)) {
        *errorString = QStringLiteral("failed to create test media files");
        return nullptr;
    }

    *imageSourcePath = thirdImage;
    *videoSourcePath = secondVideo;
    return directory;
}

std::unique_ptr<QTemporaryDir> createDirectoryCollection(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary image document directory was not created");
        return nullptr;
    }

    for (int index = 1; index <= 3; ++index) {
        const QString path
            = directory->filePath(QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0')));
        if (!writeTestPng(path)) {
            *errorString = QStringLiteral("failed to write test image %1").arg(path);
            return nullptr;
        }
    }

    *sourcePath = directory->path();
    return directory;
}

std::unique_ptr<QTemporaryDir> createComicBookArchive(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary comic archive directory was not created");
        return nullptr;
    }

    const QByteArray firstPage = encodedTestPng(Qt::blue);
    const QByteArray secondPage = encodedTestPng(Qt::green);
    if (firstPage.isEmpty() || secondPage.isEmpty()) {
        *errorString = QStringLiteral("failed to encode test archive pages");
        return nullptr;
    }

    const QString archivePath = directory->filePath(QStringLiteral("book.cbz"));
    KZip archive(archivePath);
    if (!archive.open(QIODevice::WriteOnly)) {
        *errorString = QStringLiteral("failed to open test comic archive for writing");
        return nullptr;
    }
    if (!archive.writeFile(QStringLiteral("01.png"), firstPage)
        || !archive.writeFile(QStringLiteral("02.png"), secondPage)) {
        *errorString = QStringLiteral("failed to write test comic archive pages");
        return nullptr;
    }
    archive.close();

    *sourcePath = archivePath;
    return directory;
}

MainWindowFixture createMainWindowFixture(const QUrl &initialSourceUrl);

MainWindowFixture createMainWindowFixture() { return createMainWindowFixture(QUrl()); }

MainWindowFixture createMainWindowFixture(const QUrl &initialSourceUrl)
{
    MainWindowFixture fixture;
    registerKiriViewQmlTypes();
    fixture.engine = std::make_unique<QQmlApplicationEngine>();
    addEnvironmentImportPaths(*fixture.engine);
    fixture.engine->addImportPath(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml")));
    KLocalization::setupLocalizedContext(fixture.engine.get());
    KiriView::registerApplicationImageProviders(*fixture.engine);
    if (!initialSourceUrl.isEmpty()) {
        QVariantMap initialProperties;
        initialProperties.insert(QStringLiteral("initialSourceUrl"), initialSourceUrl);
        fixture.engine->setInitialProperties(initialProperties);
    }

    fixture.engine->load(mainQmlUrl());
    if (fixture.engine->rootObjects().isEmpty()) {
        fixture.errorString = QStringLiteral("Main.qml did not create a root object");
        return fixture;
    }

    fixture.window = qobject_cast<QQuickWindow *>(fixture.engine->rootObjects().constFirst());
    if (fixture.window == nullptr) {
        fixture.errorString = QStringLiteral("Main.qml root object is not a QQuickWindow");
        return fixture;
    }

    if (!QTest::qWaitForWindowExposed(fixture.window)) {
        fixture.errorString = QStringLiteral("main window was not exposed");
        return fixture;
    }

    return fixture;
}

QQuickItem *readyProviderImage(QObject *root)
{
    const QList<QQuickItem *> items = root->findChildren<QQuickItem *>(
        QStringLiteral("providerImage"), Qt::FindChildrenRecursively);
    for (QQuickItem *item : items) {
        if (effectivelyVisible(item) && item->property("status").toInt() == 1
            && !item->property("source").toUrl().isEmpty() && item->width() > 0
            && item->height() > 0) {
            return item;
        }
    }
    return nullptr;
}

QString providerImageStateReport(QObject *root)
{
    QStringList states;
    KiriDocumentSession *documentSession = findDocumentSession(root);
    if (documentSession != nullptr && documentSession->imageDocument() != nullptr) {
        KiriImageDocument *imageDocument = documentSession->imageDocument();
        states.append(QStringLiteral("document viewport=%1x%2 display=%3x%4 primary=%5x%6")
                .arg(imageDocument->viewportSize().width())
                .arg(imageDocument->viewportSize().height())
                .arg(imageDocument->displaySize().width())
                .arg(imageDocument->displaySize().height())
                .arg(imageDocument->primaryDisplaySize().width())
                .arg(imageDocument->primaryDisplaySize().height()));
    }
    const QList<QQuickItem *> items = root->findChildren<QQuickItem *>(
        QStringLiteral("providerImage"), Qt::FindChildrenRecursively);
    for (QQuickItem *item : items) {
        QStringList ancestors;
        for (QQuickItem *ancestor = item->parentItem(); ancestor != nullptr;
            ancestor = ancestor->parentItem()) {
            ancestors.append(QStringLiteral("%1:%2x%3")
                    .arg(ancestor->objectName())
                    .arg(ancestor->width())
                    .arg(ancestor->height()));
        }
        states.append(QStringLiteral(
            "visible=%1 effectiveVisible=%2 status=%3 source=%4 size=%5x%6 ancestors=%7")
                .arg(item->isVisible())
                .arg(effectivelyVisible(item))
                .arg(item->property("status").toInt())
                .arg(item->property("source").toUrl().toString())
                .arg(item->width())
                .arg(item->height())
                .arg(ancestors.join(QStringLiteral(" > "))));
    }
    return states.join(QStringLiteral("; "));
}

void openSourceUrl(MainWindowFixture &fixture, const QString &sourcePath)
{
    KiriDocumentSession *documentSession = findDocumentSession(fixture.window);
    QVERIFY(documentSession != nullptr);
    documentSession->setSourceUrl(QUrl::fromLocalFile(sourcePath));
}

void resizeWindow(MainWindowFixture &fixture, const QSize &size)
{
    fixture.window->resize(size);
    QTRY_COMPARE(fixture.window->size(), size);
    QCoreApplication::processEvents();
}

void compareToolbarPageReadout(
    MainWindowFixture &fixture, const QString &currentText, const QString &countText, bool enabled)
{
    QQuickItem *pageNumberField = findQuickItem(fixture.window, QStringLiteral("pageNumberField"));
    QQuickItem *pageCountLabel = findQuickItem(fixture.window, QStringLiteral("pageCountLabel"));
    QQuickItem *leftPageButton
        = findQuickItem(fixture.window, QStringLiteral("leftPageNavigationButton"));
    QQuickItem *rightPageButton
        = findQuickItem(fixture.window, QStringLiteral("rightPageNavigationButton"));
    QVERIFY(pageNumberField != nullptr);
    QVERIFY(pageCountLabel != nullptr);
    QVERIFY(leftPageButton != nullptr);
    QVERIFY(rightPageButton != nullptr);

    QTRY_COMPARE(pageNumberField->property("text").toString(), currentText);
    QTRY_COMPARE(pageCountLabel->property("text").toString(), countText);
    QCOMPARE(pageNumberField->isEnabled(), enabled);
}

bool popupOpen(QObject *popup)
{
    return popup->property("visible").toBool() || popup->property("opened").toBool();
}

bool invokeBool(QObject *object, const char *method)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked && result.toBool();
}

QPoint itemCenter(QQuickItem *item)
{
    if (item == nullptr || item->width() <= 0 || item->height() <= 0) {
        return QPoint(-1, -1);
    }

    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
}

void clickItem(QQuickWindow *window, QQuickItem *item, Qt::MouseButton button)
{
    const QPoint point = itemCenter(item);
    QVERIFY(point.x() >= 0);
    QVERIFY(point.y() >= 0);
    QTest::mouseClick(window, button, Qt::NoModifier, point);
    QCoreApplication::processEvents();
}

void wheelItem(QQuickWindow *window, QQuickItem *item, int angleDeltaY)
{
    const QPoint point = itemCenter(item);
    QVERIFY(point.x() >= 0);
    QVERIFY(point.y() >= 0);

    QWheelEvent event(QPointF(point), window->mapToGlobal(point), QPoint(), QPoint(0, angleDeltaY),
        Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}

void rightButtonWheelItem(QQuickWindow *window, QQuickItem *item, int angleDeltaY)
{
    const QPoint point = itemCenter(item);
    QVERIFY(point.x() >= 0);
    QVERIFY(point.y() >= 0);

    QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, point);
    QWheelEvent event(QPointF(point), window->mapToGlobal(point), QPoint(), QPoint(0, angleDeltaY),
        Qt::RightButton, Qt::NoModifier, Qt::ScrollUpdate, false);
    QCoreApplication::sendEvent(window, &event);
    QTest::mouseRelease(window, Qt::RightButton, Qt::NoModifier, point);
    QCoreApplication::processEvents();
}

void closePopup(QObject *popup)
{
    popup->setProperty("visible", false);
    QCoreApplication::processEvents();
}

bool zoomApproximatelyEqual(double left, double right) { return std::abs(left - right) < 0.001; }
}

void TestMainWindowToolBar::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    resetConfig();

    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}

void TestMainWindowToolBar::init()
{
    resetConfig();
    QTest::failOnWarning(QRegularExpression(
        QStringLiteral(".*Created graphical object was not placed in the graphics scene.*")));
}

void TestMainWindowToolBar::startupCreatesOneVisibleToolbarWithDisabledMediaControls()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QTRY_COMPARE(controlToolBars(fixture.window).size(), 1);
    QQuickItem *toolbar = controlToolBars(fixture.window).constFirst();
    QVERIFY(toolbar->isVisible());
    QCOMPARE(toolbar->objectName(), QStringLiteral("mainImageToolBar"));

    QQuickItem *leftPageButton
        = findQuickItem(fixture.window, QStringLiteral("leftPageNavigationButton"));
    QQuickItem *rightPageButton
        = findQuickItem(fixture.window, QStringLiteral("rightPageNavigationButton"));
    QVERIFY(leftPageButton != nullptr);
    QVERIFY(rightPageButton != nullptr);
    QVERIFY(!leftPageButton->isEnabled());
    QVERIFY(!rightPageButton->isEnabled());
    compareToolbarPageReadout(fixture, QStringLiteral("–"), QStringLiteral("–"), false);

    QQuickItem *zoomSpinBox = findQuickItem(fixture.window, QStringLiteral("zoomSpinBox"));
    QQuickItem *zoomTextInput = findQuickItem(fixture.window, QStringLiteral("zoomTextInput"));
    QVERIFY(zoomSpinBox != nullptr);
    QVERIFY(zoomTextInput != nullptr);
    QVERIFY(!zoomSpinBox->isEnabled());
    QTRY_COMPARE(zoomSpinBox->property("value").toInt(), 0);
    QTRY_COMPARE(zoomTextInput->property("text").toString(), QStringLiteral("    - %"));

    const QList<QQuickItem *> visibleApplicationMenuButtons
        = visibleItemsByObjectName(fixture.window, QStringLiteral("toolbarApplicationMenuButton"));
    QCOMPARE(visibleApplicationMenuButtons.size(), 1);
    QVERIFY(visibleApplicationMenuButtons.constFirst()->isEnabled());
}

void TestMainWindowToolBar::startupInitialDirectImageRendersMainViewport()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath = directory.filePath(QStringLiteral("startup.png"));
    QVERIFY(writeTestPng(imagePath));

    MainWindowFixture fixture = createMainWindowFixture(QUrl::fromLocalFile(imagePath));
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    KiriDocumentSession *documentSession = findDocumentSession(fixture.window);
    QVERIFY(documentSession != nullptr);
    QTRY_VERIFY(documentSession->activeImageReady());
    QTRY_COMPARE(documentSession->imageDocument()->status(), KiriImageDocument::Status::Ready);

    QQuickItem *thumbnailPanel = findQuickItem(fixture.window, QStringLiteral("thumbnailPanel"));
    QVERIFY(thumbnailPanel != nullptr);
    QVERIFY(!thumbnailPanel->isVisible());

    QQuickItem *providerImage = nullptr;
    QTRY_VERIFY2((providerImage = readyProviderImage(fixture.window)) != nullptr,
        qPrintable(providerImageStateReport(fixture.window)));
    QVERIFY(!providerImage->property("source").toUrl().isEmpty());
    QVERIFY(providerImage->width() > 0);
    QVERIFY(providerImage->height() > 0);
}

void TestMainWindowToolBar::startupInitialComicArchiveRendersAndNavigatesMainViewport()
{
    QString archivePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> archiveDirectory
        = createComicBookArchive(&archivePath, &errorString);
    QVERIFY2(archiveDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture(QUrl::fromLocalFile(archivePath));
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    KiriDocumentSession *documentSession = findDocumentSession(fixture.window);
    QVERIFY(documentSession != nullptr);
    QTRY_VERIFY(documentSession->activeImageReady());
    QTRY_COMPARE(documentSession->imageDocument()->status(), KiriImageDocument::Status::Ready);
    compareToolbarPageReadout(fixture, QStringLiteral("1"), QStringLiteral("2"), true);

    QQuickItem *thumbnailPanel = findQuickItem(fixture.window, QStringLiteral("thumbnailPanel"));
    QVERIFY(thumbnailPanel != nullptr);
    QVERIFY(!thumbnailPanel->isVisible());

    QQuickItem *providerImage = nullptr;
    QTRY_VERIFY2((providerImage = readyProviderImage(fixture.window)) != nullptr,
        qPrintable(providerImageStateReport(fixture.window)));
    const QUrl firstPageSource = providerImage->property("source").toUrl();
    QVERIFY(!firstPageSource.isEmpty());

    documentSession->imageDocument()->openNextPage();
    compareToolbarPageReadout(fixture, QStringLiteral("2"), QStringLiteral("2"), true);
    QTRY_VERIFY((providerImage = readyProviderImage(fixture.window)) != nullptr
        && providerImage->property("source").toUrl() != firstPageSource);
    const QUrl secondPageSource = providerImage->property("source").toUrl();
    QVERIFY(!secondPageSource.isEmpty());

    documentSession->imageDocument()->openPreviousPage();
    compareToolbarPageReadout(fixture, QStringLiteral("1"), QStringLiteral("2"), true);
    QTRY_VERIFY((providerImage = readyProviderImage(fixture.window)) != nullptr
        && providerImage->property("source").toUrl() != secondPageSource);
    QVERIFY(!providerImage->property("source").toUrl().isEmpty());
    QVERIFY(!thumbnailPanel->isVisible());
}

void TestMainWindowToolBar::directImageShowsMediaPositionAfterSiblingListing()
{
    QString imageSourcePath;
    QString videoSourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> mediaDirectory
        = createMediaDirectory(&imageSourcePath, &videoSourcePath, &errorString);
    QVERIFY2(mediaDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    openSourceUrl(fixture, imageSourcePath);

    compareToolbarPageReadout(fixture, QStringLiteral("3"), QStringLiteral("3"), true);
}

void TestMainWindowToolBar::directoryImageDocumentShowsPagePosition()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> imageDirectory
        = createDirectoryCollection(&sourcePath, &errorString);
    QVERIFY2(imageDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    openSourceUrl(fixture, sourcePath);

    compareToolbarPageReadout(fixture, QStringLiteral("1"), QStringLiteral("3"), true);
}

void TestMainWindowToolBar::mediaViewportHostLoadsOnlyActiveDelegate()
{
    QString imageSourcePath;
    QString videoSourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> mediaDirectory
        = createMediaDirectory(&imageSourcePath, &videoSourcePath, &errorString);
    QVERIFY2(mediaDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QVERIFY(findQuickItem(fixture.window, QStringLiteral("mediaViewportSlot")) != nullptr);
    QVERIFY(findQuickItem(fixture.window, QStringLiteral("imageViewport")) == nullptr);
    QVERIFY(findQuickItem(fixture.window, QStringLiteral("videoViewport")) == nullptr);

    openSourceUrl(fixture, imageSourcePath);
    QTRY_VERIFY(findQuickItem(fixture.window, QStringLiteral("imageViewport")) != nullptr);
    QVERIFY(findQuickItem(fixture.window, QStringLiteral("videoViewport")) == nullptr);

    openSourceUrl(fixture, videoSourcePath);
    QTRY_VERIFY(findQuickItem(fixture.window, QStringLiteral("videoViewport")) != nullptr);
    QTRY_VERIFY(findQuickItem(fixture.window, QStringLiteral("imageViewport")) == nullptr);

    openSourceUrl(fixture, imageSourcePath);
    QTRY_VERIFY(findQuickItem(fixture.window, QStringLiteral("imageViewport")) != nullptr);
    QTRY_VERIFY(findQuickItem(fixture.window, QStringLiteral("videoViewport")) == nullptr);
}

void TestMainWindowToolBar::panelActionsToggleResizablePanels()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    resizeWindow(fixture, QSize(1200, 800));

    KiriViewApplication *application = findApplication(fixture.window);
    QVERIFY(application != nullptr);
    QAction *infoPanelAction
        = application->actionForId(KiriViewApplication::ViewToggleInfoPanelAction);
    QAction *thumbnailPanelAction
        = application->actionForId(KiriViewApplication::ViewToggleThumbnailPanelAction);
    QVERIFY(infoPanelAction != nullptr);
    QVERIFY(thumbnailPanelAction != nullptr);

    QQuickItem *contentSplitView
        = findQuickItem(fixture.window, QStringLiteral("contentSplitView"));
    QQuickItem *mediaPanelSplitView
        = findQuickItem(fixture.window, QStringLiteral("mediaPanelSplitView"));
    QQuickItem *infoPanel = findQuickItem(fixture.window, QStringLiteral("infoPanel"));
    QQuickItem *infoPanelOverlay
        = findQuickItem(fixture.window, QStringLiteral("infoPanelOverlayContent"));
    QQuickItem *thumbnailPanel = findQuickItem(fixture.window, QStringLiteral("thumbnailPanel"));
    QVERIFY(contentSplitView != nullptr);
    QVERIFY(mediaPanelSplitView != nullptr);
    QVERIFY(infoPanel != nullptr);
    QVERIFY(infoPanelOverlay != nullptr);
    QVERIFY(thumbnailPanel != nullptr);
    QVERIFY(!infoPanel->isVisible());
    QVERIFY(!infoPanelOverlay->isVisible());
    QVERIFY(!thumbnailPanel->isVisible());

    infoPanelAction->trigger();
    QTRY_VERIFY(infoPanel->isVisible());
    QVERIFY(!infoPanelOverlay->isVisible());
    QVERIFY(!thumbnailPanel->isVisible());
    QVERIFY(infoPanel->width() > 0);
    QVERIFY(infoPanel->width() >= 16 * 16);
    QVERIFY(infoPanel->width() <= 20 * 32);
    QTRY_VERIFY(qAbs(infoPanel->height() - contentSplitView->height()) <= 1.0);

    thumbnailPanelAction->trigger();
    QTRY_VERIFY(thumbnailPanel->isVisible());
    QVERIFY(thumbnailPanel->height() > 0);
    QVERIFY(thumbnailPanel->width() <= mediaPanelSplitView->width());

    fixture.window->setVisibility(QWindow::FullScreen);
    QTRY_COMPARE(fixture.window->visibility(), QWindow::FullScreen);
    QTRY_VERIFY(infoPanel->isVisible());
    QTRY_VERIFY(thumbnailPanel->isVisible());

    infoPanelAction->trigger();
    thumbnailPanelAction->trigger();
    QTRY_VERIFY(!infoPanel->isVisible());
    QTRY_VERIFY(!infoPanelOverlay->isVisible());
    QTRY_VERIFY(!thumbnailPanel->isVisible());
}

void TestMainWindowToolBar::infoPanelUsesOverlayDrawerOnNarrowWindows()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    resizeWindow(fixture, QSize(520, 420));

    KiriViewApplication *application = findApplication(fixture.window);
    QVERIFY(application != nullptr);
    QAction *infoPanelAction
        = application->actionForId(KiriViewApplication::ViewToggleInfoPanelAction);
    QVERIFY(infoPanelAction != nullptr);

    QQuickItem *mediaViewportSlot
        = findQuickItem(fixture.window, QStringLiteral("mediaViewportSlot"));
    QQuickItem *inlineInfoPanel = findQuickItem(fixture.window, QStringLiteral("infoPanel"));
    QQuickItem *overlayInfoPanel
        = findQuickItem(fixture.window, QStringLiteral("infoPanelOverlayContent"));
    QObject *overlayDrawer = findObject(fixture.window, QStringLiteral("infoPanelOverlayDrawer"));
    QVERIFY(mediaViewportSlot != nullptr);
    QVERIFY(inlineInfoPanel != nullptr);
    QVERIFY(overlayInfoPanel != nullptr);
    QVERIFY(overlayDrawer != nullptr);
    const qreal viewportWidthBeforeOpen = mediaViewportSlot->width();

    infoPanelAction->trigger();

    QTRY_VERIFY(!inlineInfoPanel->isVisible());
    QTRY_VERIFY(overlayInfoPanel->isVisible());
    QVERIFY(overlayDrawer->property("drawerOpen").toBool());
    QVERIFY(qAbs(mediaViewportSlot->width() - viewportWidthBeforeOpen) <= 1.0);

    const QList<QQuickItem *> closeButtons
        = visibleItemsByObjectName(fixture.window, QStringLiteral("infoPanelCloseButton"));
    QCOMPARE(closeButtons.size(), 1);
    QTRY_VERIFY(itemCenter(closeButtons.constFirst()).x() < fixture.window->width());
    clickItem(fixture.window, closeButtons.constFirst(), Qt::LeftButton);
    QTRY_VERIFY(!overlayInfoPanel->isVisible());
    QVERIFY(!overlayDrawer->property("drawerOpen").toBool());
}

void TestMainWindowToolBar::escapeClosesInfoPanelBeforeLeavingFullscreen()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    resizeWindow(fixture, QSize(1200, 800));

    KiriViewApplication *application = findApplication(fixture.window);
    QVERIFY(application != nullptr);
    QAction *infoPanelAction
        = application->actionForId(KiriViewApplication::ViewToggleInfoPanelAction);
    QVERIFY(infoPanelAction != nullptr);

    fixture.window->setVisibility(QWindow::FullScreen);
    QTRY_COMPARE(fixture.window->visibility(), QWindow::FullScreen);

    QQuickItem *infoPanel = findQuickItem(fixture.window, QStringLiteral("infoPanel"));
    QVERIFY(infoPanel != nullptr);
    infoPanelAction->trigger();
    QTRY_VERIFY(infoPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_Escape);
    QTRY_VERIFY(!infoPanel->isVisible());
    QCOMPARE(fixture.window->visibility(), QWindow::FullScreen);

    QTest::keyClick(fixture.window, Qt::Key_Escape);
    QTRY_COMPARE(fixture.window->visibility(), QWindow::Windowed);
}

void TestMainWindowToolBar::panelShortcutsToggleResizablePanels()
{
    QString imageSourcePath;
    QString videoSourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> mediaDirectory
        = createMediaDirectory(&imageSourcePath, &videoSourcePath, &errorString);
    QVERIFY2(mediaDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    resizeWindow(fixture, QSize(1200, 800));

    openSourceUrl(fixture, imageSourcePath);
    QTRY_VERIFY(findQuickItem(fixture.window, QStringLiteral("imageViewport")) != nullptr);

    QQuickItem *infoPanel = findQuickItem(fixture.window, QStringLiteral("infoPanel"));
    QQuickItem *thumbnailPanel = findQuickItem(fixture.window, QStringLiteral("thumbnailPanel"));
    QVERIFY(infoPanel != nullptr);
    QVERIFY(thumbnailPanel != nullptr);
    QVERIFY(!infoPanel->isVisible());
    QVERIFY(!thumbnailPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_I, Qt::ControlModifier);
    QCoreApplication::processEvents();
    QVERIFY(!infoPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_T, Qt::ControlModifier);
    QCoreApplication::processEvents();
    QVERIFY(!thumbnailPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_I);
    QTRY_VERIFY(infoPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_T);
    QTRY_VERIFY(thumbnailPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_I);
    QTRY_VERIFY(!infoPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_T);
    QTRY_VERIFY(!thumbnailPanel->isVisible());
}

void TestMainWindowToolBar::commandFixedShortcutsUseApplicationActions()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    KiriViewApplication *application = findApplication(fixture.window);
    QQuickItem *toolbar = findQuickItem(fixture.window, QStringLiteral("mainImageToolBar"));
    QVERIFY(application != nullptr);
    QVERIFY(toolbar != nullptr);

    QAction *showMenubarAction
        = application->actionForId(KiriViewApplication::OptionsShowMenubarAction);
    QAction *openApplicationMenuAction
        = application->actionForId(KiriViewApplication::OpenApplicationMenuAction);
    QVERIFY(showMenubarAction != nullptr);
    QVERIFY(openApplicationMenuAction != nullptr);
    QSignalSpy showMenubarSpy(showMenubarAction, &QAction::triggered);
    QSignalSpy openApplicationMenuSpy(openApplicationMenuAction, &QAction::triggered);

    QCOMPARE(application->menuPresentation(), KiriViewApplication::HamburgerMenu);
    QVERIFY(!invokeBool(toolbar, "applicationMenuOpen"));

    QTest::keyClick(fixture.window, Qt::Key_M);
    QCoreApplication::processEvents();
    QCOMPARE(showMenubarSpy.count(), 0);
    QCOMPARE(application->menuPresentation(), KiriViewApplication::HamburgerMenu);

    QTest::keyClick(fixture.window, Qt::Key_M, Qt::ControlModifier);
    QTRY_COMPARE(showMenubarSpy.count(), 1);
    QCOMPARE(application->menuPresentation(), KiriViewApplication::MenuBar);

    QTest::keyClick(fixture.window, Qt::Key_F10);
    QCoreApplication::processEvents();
    QCOMPARE(openApplicationMenuSpy.count(), 0);
    QVERIFY(!invokeBool(toolbar, "applicationMenuOpen"));

    QTest::keyClick(fixture.window, Qt::Key_M, Qt::ControlModifier);
    QTRY_COMPARE(showMenubarSpy.count(), 2);
    QCOMPARE(application->menuPresentation(), KiriViewApplication::HamburgerMenu);

    QTest::keyClick(fixture.window, Qt::Key_F10);
    QTRY_COMPARE(openApplicationMenuSpy.count(), 1);
    QTRY_VERIFY(invokeBool(toolbar, "applicationMenuOpen"));
}

void TestMainWindowToolBar::viewerRightClickOpensContextMenuOnlyFromMediaViewport()
{
    QString imageSourcePath;
    QString videoSourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> mediaDirectory
        = createMediaDirectory(&imageSourcePath, &videoSourcePath, &errorString);
    QVERIFY2(mediaDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    resizeWindow(fixture, QSize(1200, 800));
    openSourceUrl(fixture, imageSourcePath);
    compareToolbarPageReadout(fixture, QStringLiteral("3"), QStringLiteral("3"), true);

    KiriDocumentSession *documentSession = findDocumentSession(fixture.window);
    QVERIFY(documentSession != nullptr);
    KiriImageDocument *imageDocument = documentSession->imageDocument();
    QVERIFY(imageDocument != nullptr);
    QTRY_COMPARE(imageDocument->status(), KiriImageDocument::Status::Ready);
    QTRY_VERIFY(documentSession->activeImageReady());

    QObject *contextMenu = findObject(fixture.window, QStringLiteral("viewerContextMenu"));
    QQuickItem *mediaViewportSlot
        = findQuickItem(fixture.window, QStringLiteral("mediaViewportSlot"));
    QQuickItem *toolbar = findQuickItem(fixture.window, QStringLiteral("mainImageToolBar"));
    QVERIFY(contextMenu != nullptr);
    QVERIFY(mediaViewportSlot != nullptr);
    QVERIFY(toolbar != nullptr);
    QVERIFY(!popupOpen(contextMenu));
    QCOMPARE(contextMenu->property("actionCount").toInt(), 22);

    clickItem(fixture.window, toolbar, Qt::RightButton);
    QTRY_VERIFY(!popupOpen(contextMenu));

    clickItem(fixture.window, mediaViewportSlot, Qt::RightButton);
    QTRY_VERIFY(popupOpen(contextMenu));
    closePopup(contextMenu);
    QTRY_VERIFY(!popupOpen(contextMenu));

    KiriViewApplication *application = findApplication(fixture.window);
    QVERIFY(application != nullptr);
    QAction *infoPanelAction
        = application->actionForId(KiriViewApplication::ViewToggleInfoPanelAction);
    QAction *thumbnailPanelAction
        = application->actionForId(KiriViewApplication::ViewToggleThumbnailPanelAction);
    QVERIFY(infoPanelAction != nullptr);
    QVERIFY(thumbnailPanelAction != nullptr);
    infoPanelAction->trigger();
    thumbnailPanelAction->trigger();

    QQuickItem *infoPanel = findQuickItem(fixture.window, QStringLiteral("infoPanel"));
    QQuickItem *thumbnailPanel = findQuickItem(fixture.window, QStringLiteral("thumbnailPanel"));
    QVERIFY(infoPanel != nullptr);
    QVERIFY(thumbnailPanel != nullptr);
    QTRY_VERIFY(infoPanel->isVisible());
    QTRY_VERIFY(thumbnailPanel->isVisible());

    clickItem(fixture.window, infoPanel, Qt::RightButton);
    QTRY_VERIFY(!popupOpen(contextMenu));
    clickItem(fixture.window, thumbnailPanel, Qt::RightButton);
    QTRY_VERIFY(!popupOpen(contextMenu));

    infoPanelAction->trigger();
    thumbnailPanelAction->trigger();
    QTRY_VERIFY(!infoPanel->isVisible());
    QTRY_VERIFY(!thumbnailPanel->isVisible());

    resizeWindow(fixture, QSize(520, 420));
    fixture.window->setVisibility(QWindow::FullScreen);
    QTRY_COMPARE(fixture.window->visibility(), QWindow::FullScreen);
    clickItem(fixture.window, mediaViewportSlot, Qt::RightButton);
    QTRY_VERIFY(popupOpen(contextMenu));
}

void TestMainWindowToolBar::toolbarZoomWheelAppliesFineManualStep()
{
    QString imageSourcePath;
    QString videoSourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> mediaDirectory
        = createMediaDirectory(&imageSourcePath, &videoSourcePath, &errorString);
    QVERIFY2(mediaDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    openSourceUrl(fixture, imageSourcePath);

    KiriDocumentSession *documentSession = findDocumentSession(fixture.window);
    QVERIFY(documentSession != nullptr);
    KiriImageDocument *imageDocument = documentSession->imageDocument();
    QVERIFY(imageDocument != nullptr);
    QTRY_COMPARE(imageDocument->status(), KiriImageDocument::Status::Ready);

    QQuickItem *zoomSpinBox = findQuickItem(fixture.window, QStringLiteral("zoomSpinBox"));
    QVERIFY(zoomSpinBox != nullptr);
    QTRY_VERIFY(zoomSpinBox->isEnabled());

    QVERIFY(imageDocument->requestManualZoomPercent(100.0));
    QTRY_VERIFY(zoomApproximatelyEqual(imageDocument->zoomPercent(), 100.0));
    const double zoomedInPercent = imageDocument->steppedManualZoomPercent(0.5);

    wheelItem(fixture.window, zoomSpinBox, 120);
    QTRY_VERIFY(zoomApproximatelyEqual(imageDocument->zoomPercent(), zoomedInPercent));

    wheelItem(fixture.window, zoomSpinBox, -120);
    QTRY_VERIFY(zoomApproximatelyEqual(imageDocument->zoomPercent(), 100.0));
}

void TestMainWindowToolBar::rightButtonWheelSuppressesContextMenuTap()
{
    QString imageSourcePath;
    QString videoSourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> mediaDirectory
        = createMediaDirectory(&imageSourcePath, &videoSourcePath, &errorString);
    QVERIFY2(mediaDirectory != nullptr, qPrintable(errorString));

    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    openSourceUrl(fixture, imageSourcePath);
    compareToolbarPageReadout(fixture, QStringLiteral("3"), QStringLiteral("3"), true);

    QObject *contextMenu = findObject(fixture.window, QStringLiteral("viewerContextMenu"));
    QQuickItem *mediaViewportSlot
        = findQuickItem(fixture.window, QStringLiteral("mediaViewportSlot"));
    QVERIFY(contextMenu != nullptr);
    QVERIFY(mediaViewportSlot != nullptr);
    QVERIFY(!popupOpen(contextMenu));

    rightButtonWheelItem(fixture.window, mediaViewportSlot, 120);
    QTRY_VERIFY(!popupOpen(contextMenu));

    clickItem(fixture.window, mediaViewportSlot, Qt::RightButton);
    QTRY_VERIFY(popupOpen(contextMenu));
}

void TestMainWindowToolBar::fullscreenReusesSingleToolbarAndHidesApplicationMenuButton()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_COMPARE(controlToolBars(fixture.window).size(), 1);
    QQuickItem *windowedToolbar = controlToolBars(fixture.window).constFirst();

    fixture.window->setVisibility(QWindow::FullScreen);
    QTRY_COMPARE(fixture.window->visibility(), QWindow::FullScreen);

    QTRY_COMPARE(controlToolBars(fixture.window).size(), 1);
    QCOMPARE(controlToolBars(fixture.window).constFirst(), windowedToolbar);
    QVERIFY(windowedToolbar->isVisible());

    const QList<QQuickItem *> visibleApplicationMenuButtons
        = visibleItemsByObjectName(fixture.window, QStringLiteral("toolbarApplicationMenuButton"));
    QVERIFY(visibleApplicationMenuButtons.isEmpty());
}

QTEST_MAIN(TestMainWindowToolBar)

#include "test_mainwindowtoolbar.moc"
