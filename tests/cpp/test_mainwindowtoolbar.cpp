// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationruntime.h"
#include "facade/imageactionavailability.h"
#include "facade/imageshortcutnavigationpolicy.h"
#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedocument.h"
#include "facade/kiriimageview.h"
#include "facade/kirivideodocument.h"
#include "facade/kiriviewapplication.h"
#include "facade/menuaccesskeyrouter.h"
#include "kiriviewstate.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
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
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QWheelEvent>
#include <QtQml/qqml.h>
#include <memory>

class TestMainWindowToolBar : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void startupCreatesOneVisibleToolbarWithDisabledMediaControls();
    void directImageShowsMediaPositionAfterSiblingListing();
    void directoryImageDocumentShowsPagePosition();
    void mediaViewportHostLoadsOnlyActiveDelegate();
    void panelActionsToggleResizablePanels();
    void panelShortcutsToggleResizablePanels();
    void commandFixedShortcutsUseApplicationActions();
    void viewerRightClickOpensContextMenuOnlyFromMediaViewport();
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
    qmlRegisterType<ImageShortcutNavigationPolicy>(
        "io.github.hnjae.kiriview", 1, 0, "ImageShortcutNavigationPolicy");
    qmlRegisterType<KiriDocumentSession>("io.github.hnjae.kiriview", 1, 0, "KiriDocumentSession");
    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<KiriImageView>("io.github.hnjae.kiriview", 1, 0, "KiriImageView");
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

QList<QQuickItem *> visibleItemsByObjectName(QObject *root, const QString &objectName)
{
    QList<QQuickItem *> visibleItems;
    const QList<QQuickItem *> items
        = root->findChildren<QQuickItem *>(objectName, Qt::FindChildrenRecursively);
    for (QQuickItem *item : items) {
        if (item->isVisible()) {
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

MainWindowFixture createMainWindowFixture()
{
    MainWindowFixture fixture;
    registerKiriViewQmlTypes();
    fixture.engine = std::make_unique<QQmlApplicationEngine>();
    addEnvironmentImportPaths(*fixture.engine);
    fixture.engine->addImportPath(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml")));
    KLocalization::setupLocalizedContext(fixture.engine.get());

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

void openSourceUrl(MainWindowFixture &fixture, const QString &sourcePath)
{
    KiriDocumentSession *documentSession = findDocumentSession(fixture.window);
    QVERIFY(documentSession != nullptr);
    documentSession->setSourceUrl(QUrl::fromLocalFile(sourcePath));
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
    QQuickItem *thumbnailPanel = findQuickItem(fixture.window, QStringLiteral("thumbnailPanel"));
    QVERIFY(contentSplitView != nullptr);
    QVERIFY(mediaPanelSplitView != nullptr);
    QVERIFY(infoPanel != nullptr);
    QVERIFY(thumbnailPanel != nullptr);
    QVERIFY(!infoPanel->isVisible());
    QVERIFY(!thumbnailPanel->isVisible());

    infoPanelAction->trigger();
    QTRY_VERIFY(infoPanel->isVisible());
    QVERIFY(!thumbnailPanel->isVisible());
    QVERIFY(infoPanel->width() > 0);
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
    QTRY_VERIFY(!thumbnailPanel->isVisible());
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

    openSourceUrl(fixture, imageSourcePath);
    QTRY_VERIFY(findQuickItem(fixture.window, QStringLiteral("imageViewport")) != nullptr);

    QQuickItem *infoPanel = findQuickItem(fixture.window, QStringLiteral("infoPanel"));
    QQuickItem *thumbnailPanel = findQuickItem(fixture.window, QStringLiteral("thumbnailPanel"));
    QVERIFY(infoPanel != nullptr);
    QVERIFY(thumbnailPanel != nullptr);
    QVERIFY(!infoPanel->isVisible());
    QVERIFY(!thumbnailPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_I, Qt::ControlModifier);
    QTRY_VERIFY(infoPanel->isVisible());

    QTest::keyClick(fixture.window, Qt::Key_T, Qt::ControlModifier);
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
    openSourceUrl(fixture, imageSourcePath);
    compareToolbarPageReadout(fixture, QStringLiteral("3"), QStringLiteral("3"), true);

    QObject *contextMenu = findObject(fixture.window, QStringLiteral("viewerContextMenu"));
    QQuickItem *mediaViewportSlot
        = findQuickItem(fixture.window, QStringLiteral("mediaViewportSlot"));
    QQuickItem *toolbar = findQuickItem(fixture.window, QStringLiteral("mainImageToolBar"));
    QVERIFY(contextMenu != nullptr);
    QVERIFY(mediaViewportSlot != nullptr);
    QVERIFY(toolbar != nullptr);
    QVERIFY(!popupOpen(contextMenu));
    QCOMPARE(contextMenu->property("actionCount").toInt(), 20);

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

    fixture.window->setVisibility(QWindow::FullScreen);
    QTRY_COMPARE(fixture.window->visibility(), QWindow::FullScreen);
    clickItem(fixture.window, mediaViewportSlot, Qt::RightButton);
    QTRY_VERIFY(popupOpen(contextMenu));
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
