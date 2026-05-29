// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/imageactionavailability.h"
#include "facade/imageshortcutnavigationpolicy.h"
#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirimediainformation.h"
#include "facade/kirivideodocument.h"
#include "facade/kiriviewapplication.h"
#include "kiriviewstate.h"
#include "localization/localization.h"

#include <KConfigGroup>
#include <KLocalizedQmlContext>
#include <KSharedConfig>
#include <KZip>
#include <QAction>
#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>
#include <memory>

class TestImageShortcuts : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void arrowKeysPanAsFixedViewerShortcuts();
    void leftAndRightArrowKeysUseNavigationFallback();
    void arrowKeysAreIgnoredWhileViewerShortcutsAreSuppressed();
    void shiftArrowsMoveOnePageInTwoPageModeLeftToRight();
    void shiftArrowsMoveOnePageInTwoPageModeRightToLeft();
    void shiftArrowsAreIgnoredWhileViewerShortcutsAreSuppressed();
    void configuredActionShortcutsTriggerActions();
    void videoViewerAliasTriggersFullscreenAction();
    void videoImageOnlyShortcutsShowUnsupportedToast();
};

namespace {
struct ImageShortcutsFixture {
    std::unique_ptr<QQuickView> view;
    std::unique_ptr<QTemporaryDir> temporaryDirectory;
    QObject *root = nullptr;
    KiriViewApplication *application = nullptr;
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr && application != nullptr; }
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
    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<KiriDocumentSession>("io.github.hnjae.kiriview", 1, 0, "KiriDocumentSession");
    qmlRegisterType<KiriMediaInformation>("io.github.hnjae.kiriview", 1, 0, "KiriMediaInformation");
    qmlRegisterType<KiriVideoDocument>("io.github.hnjae.kiriview", 1, 0, "KiriVideoDocument");
    qmlRegisterType<ImageActionAvailability>(
        "io.github.hnjae.kiriview", 1, 0, "ImageActionAvailability");
    qmlRegisterType<ImageShortcutNavigationPolicy>(
        "io.github.hnjae.kiriview", 1, 0, "ImageShortcutNavigationPolicy");
    qmlRegisterType<KiriViewApplication>("io.github.hnjae.kiriview", 1, 0, "KiriViewApplication");
    registered = true;
}

void resetConfig()
{
    KiriViewState *state = KiriViewState::self();
    state->config()->deleteGroup(QStringLiteral("Interface"));
    state->config()->sync();
    state->config()->reparseConfiguration();
    state->read();

    KSharedConfig::Ptr appConfig = KSharedConfig::openConfig();
    appConfig->deleteGroup(QStringLiteral("Shortcuts"));
    appConfig->sync();
    appConfig->reparseConfiguration();
}

QString qmlSourceImport()
{
    const QString qmlPath = QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
                                .absoluteFilePath(QStringLiteral("../../src/qml"));
    return QUrl::fromLocalFile(qmlPath).toString();
}

bool writeTestPng(const QString &path)
{
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    return image.save(path, "PNG");
}

std::unique_ptr<QTemporaryDir> createImageDirectory(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary image directory was not created");
        return nullptr;
    }

    const QString imagePath = directory->filePath(QStringLiteral("image.png"));
    if (!writeTestPng(imagePath)) {
        *errorString = QStringLiteral("failed to write test image");
        return nullptr;
    }

    *sourcePath = imagePath;
    return directory;
}

std::unique_ptr<QTemporaryDir> createComicBookArchive(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary archive directory was not created");
        return nullptr;
    }

    QImage image(QSize(2, 4), QImage::Format_RGBA8888);
    image.fill(Qt::blue);
    QByteArray imageData;
    QBuffer imageBuffer(&imageData);
    if (!imageBuffer.open(QIODevice::WriteOnly) || !image.save(&imageBuffer, "PNG")) {
        *errorString = QStringLiteral("failed to encode test archive image");
        return nullptr;
    }

    const QString archivePath = directory->filePath(QStringLiteral("book.cbz"));
    KZip archive(archivePath);
    if (!archive.open(QIODevice::WriteOnly)) {
        *errorString = QStringLiteral("failed to open test archive for writing");
        return nullptr;
    }

    for (int index = 1; index <= 4; ++index) {
        const QString entryName = QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0'));
        if (!archive.writeFile(entryName, imageData)) {
            *errorString = QStringLiteral("failed to write test archive entry %1").arg(entryName);
            archive.close();
            return nullptr;
        }
    }
    archive.close();

    *sourcePath = archivePath;
    return directory;
}

QString fixtureQml(const QString &sourceUrl = QString())
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml

Item {
    id: root

    focus: true
    width: 320
    height: 240

    property bool helpDialogOpen: false
    property bool imageHorizontallyPannable: false
    property bool imagePannable: false
    property bool toolbarTextInputFocused: false
    property bool videoFileDeletionInProgress: false
    property bool videoMode: false
    property int panCount: 0
    property int unsupportedVideoActionCount: 0
    property string lastBoundaryMessage: ""
    property alias activeNavigationKnown: documentSession.activeNavigationKnown
    property real lastPanX: 0
    property real lastPanY: 0
    property alias currentPageNumber: imageDocument.currentPageNumber
    property alias currentLastPageNumber: imageDocument.currentLastPageNumber
    property alias pageCount: imageDocument.pageCount
    property alias rightToLeftReadingAvailable: imageDocument.rightToLeftReadingAvailable
    property alias rightToLeftReadingEnabled: imageDocument.rightToLeftReadingEnabled
    property alias secondaryPageVisible: imageDocument.secondaryPageVisible
    property KiriImageDocument testImageDocument: imageDocument
    property alias twoPageModeAvailable: imageDocument.twoPageModeAvailable
    property alias twoPageModeEnabled: imageDocument.twoPageModeEnabled

    function openImageAtPage(pageNumber) {
        imageDocument.openImageAtPage(pageNumber);
    }

    function documentReady() {
        return imageDocument.status === KiriImageDocument.Ready;
    }

    function resetPanLog() {
        panCount = 0;
        lastPanX = 0;
        lastPanY = 0;
    }

    Component.onCompleted: forceActiveFocus()

    KiriViewApplication {
        id: application

        objectName: "application"
    }

    KiriDocumentSession {
        id: documentSession

        sourceUrl: "%2"
    }

    KiriImageDocument {
        id: imageDocument

        sourceUrl: "%2"
    }

    KiriViewQml.ImageViewportInteractionSurface {
        id: imageInteractionSurface

        imageHorizontallyPannable: root.imageHorizontallyPannable
        imagePannable: root.imagePannable
        viewportHeight: 100
        viewportWidth: 100

        function panBy(deltaX, deltaY) {
            root.panCount += 1;
            root.lastPanX = deltaX;
            root.lastPanY = deltaY;
            return true;
        }

        function panToBottomRight() {
            return false;
        }

        function panToTopLeft() {
            return false;
        }

        function scanBackward() {
            return false;
        }

        function scanForward() {
            return false;
        }

        function setNextDisplayedImageStartToFinalScanPosition() {
        }

        function zoomByStep(stepCount, viewportX, viewportY) {
            return false;
        }

        function zoomByStepAtCenter(stepCount) {
            return zoomByStep(stepCount, viewportWidth / 2, viewportHeight / 2);
        }
    }

    QtObject {
        id: imageToolBar

        function textInputFocused() {
            return root.toolbarTextInputFocused;
        }
    }

    ImageActionAvailability {
        id: actionAvailability

        containerNavigationAvailable: imageDocument.containerNavigationAvailable
        fileDeletionInProgress: imageDocument.fileDeletionInProgress
        helpDialogOpen: root.helpDialogOpen
        imagePannable: imageInteractionSurface.imagePannable
        imageReady: imageDocument.status === KiriImageDocument.Ready
        rightToLeftReadingAvailable: imageDocument.rightToLeftReadingAvailable
        rightToLeftReadingEnabled: imageDocument.rightToLeftReadingEnabled
        textInputFocused: imageToolBar.textInputFocused()
        twoPageModeAvailable: imageDocument.twoPageModeAvailable
        twoPageModeEnabled: imageDocument.twoPageModeEnabled
    }

    KiriViewQml.ImageShortcuts {
        application: application
        actionAvailability: actionAvailability
        documentSession: documentSession
        imageDocument: root.testImageDocument
        imageInteractionSurface: imageInteractionSurface
        videoFileDeletionInProgress: root.videoFileDeletionInProgress
        videoMode: root.videoMode

        onImageBoundaryReached: function (message) {
            root.lastBoundaryMessage = message;
        }

        onUnsupportedVideoActionRequested: root.unsupportedVideoActionCount += 1
    }
}
)")
        .arg(qmlSourceImport(), sourceUrl);
}

ImageShortcutsFixture createFixture(const QString &sourceUrl = QString())
{
    ImageShortcutsFixture fixture;
    registerKiriViewQmlTypes();
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(320, 240);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());
    KLocalization::setupLocalizedContext(fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(
        fixtureQml(sourceUrl).toUtf8(), QUrl(QStringLiteral("memory:test_imageshortcuts.qml")));
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

    QObject *root = component.create();
    if (root == nullptr) {
        fixture.errorString = component.errorString();
        return fixture;
    }

    fixture.view->setContent(
        QUrl(QStringLiteral("memory:test_imageshortcuts.qml")), &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    fixture.application = root->findChild<KiriViewApplication *>(QStringLiteral("application"));
    if (fixture.application == nullptr) {
        fixture.errorString = QStringLiteral("application was not created");
    }
    return fixture;
}

ImageShortcutsFixture createReadyFixture()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> directory = createImageDirectory(&sourcePath, &errorString);
    if (directory == nullptr) {
        ImageShortcutsFixture fixture;
        fixture.errorString = errorString;
        return fixture;
    }

    ImageShortcutsFixture fixture = createFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(directory);
    return fixture;
}

ImageShortcutsFixture createComicBookFixture()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> directory = createComicBookArchive(&sourcePath, &errorString);
    if (directory == nullptr) {
        ImageShortcutsFixture fixture;
        fixture.errorString = errorString;
        return fixture;
    }

    ImageShortcutsFixture fixture = createFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(directory);
    return fixture;
}

bool documentReady(QObject *root)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        root, "documentReady", Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked && result.toBool();
}

void pressKey(QQuickView *view, Qt::Key key)
{
    QTest::keyClick(view, key);
    QCoreApplication::processEvents();
}

void pressKey(QQuickView *view, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    QTest::keyClick(view, key, modifiers);
    QCoreApplication::processEvents();
}

bool openImageAtPage(QObject *root, int pageNumber)
{
    return QMetaObject::invokeMethod(
        root, "openImageAtPage", Qt::DirectConnection, Q_ARG(QVariant, QVariant(pageNumber)));
}

void prepareTwoPageSpread(ImageShortcutsFixture &fixture)
{
    QVERIFY(fixture.isValid());
    QTRY_VERIFY(documentReady(fixture.root));
    QTRY_COMPARE(fixture.root->property("pageCount").toInt(), 4);
    QTRY_VERIFY(fixture.root->property("twoPageModeAvailable").toBool());
    QTRY_VERIFY(fixture.root->property("rightToLeftReadingAvailable").toBool());
    QVERIFY(fixture.root->setProperty("twoPageModeEnabled", true));
    QVERIFY(openImageAtPage(fixture.root, 2));
    QTRY_COMPARE(fixture.root->property("currentPageNumber").toInt(), 2);
    QTRY_COMPARE(fixture.root->property("currentLastPageNumber").toInt(), 3);
    QTRY_VERIFY(fixture.root->property("secondaryPageVisible").toBool());
}
}

void TestImageShortcuts::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KiriView::initializeLocalization();
    resetConfig();
}

void TestImageShortcuts::init() { resetConfig(); }

void TestImageShortcuts::cleanup() { resetConfig(); }

void TestImageShortcuts::arrowKeysPanAsFixedViewerShortcuts()
{
    ImageShortcutsFixture fixture = createReadyFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(documentReady(fixture.root));

    fixture.root->setProperty("imageHorizontallyPannable", true);
    pressKey(fixture.view.get(), Qt::Key_Left);
    QCOMPARE(fixture.root->property("panCount").toInt(), 1);
    QCOMPARE(fixture.root->property("lastPanX").toReal(), -64.0);
    QCOMPARE(fixture.root->property("lastPanY").toReal(), 0.0);

    pressKey(fixture.view.get(), Qt::Key_Right);
    QCOMPARE(fixture.root->property("panCount").toInt(), 2);
    QCOMPARE(fixture.root->property("lastPanX").toReal(), 64.0);
    QCOMPARE(fixture.root->property("lastPanY").toReal(), 0.0);

    fixture.root->setProperty("imagePannable", true);
    pressKey(fixture.view.get(), Qt::Key_Up);
    QCOMPARE(fixture.root->property("panCount").toInt(), 3);
    QCOMPARE(fixture.root->property("lastPanX").toReal(), 0.0);
    QCOMPARE(fixture.root->property("lastPanY").toReal(), -64.0);

    pressKey(fixture.view.get(), Qt::Key_Down);
    QCOMPARE(fixture.root->property("panCount").toInt(), 4);
    QCOMPARE(fixture.root->property("lastPanX").toReal(), 0.0);
    QCOMPARE(fixture.root->property("lastPanY").toReal(), 64.0);
}

void TestImageShortcuts::leftAndRightArrowKeysUseNavigationFallback()
{
    ImageShortcutsFixture fixture = createReadyFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(documentReady(fixture.root));
    QTRY_VERIFY(fixture.root->property("activeNavigationKnown").toBool());

    pressKey(fixture.view.get(), Qt::Key_Left);
    QCOMPARE(fixture.root->property("lastBoundaryMessage").toString(),
        QStringLiteral("First media item"));

    fixture.root->setProperty("lastBoundaryMessage", QString());
    pressKey(fixture.view.get(), Qt::Key_Right);
    QCOMPARE(fixture.root->property("lastBoundaryMessage").toString(),
        QStringLiteral("Last media item"));
}

void TestImageShortcuts::arrowKeysAreIgnoredWhileViewerShortcutsAreSuppressed()
{
    ImageShortcutsFixture fixture = createReadyFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(documentReady(fixture.root));
    fixture.root->setProperty("imageHorizontallyPannable", true);
    fixture.root->setProperty("imagePannable", true);

    fixture.root->setProperty("toolbarTextInputFocused", true);
    pressKey(fixture.view.get(), Qt::Key_Left);
    pressKey(fixture.view.get(), Qt::Key_Up);
    QCOMPARE(fixture.root->property("panCount").toInt(), 0);

    fixture.root->setProperty("toolbarTextInputFocused", false);
    fixture.root->setProperty("helpDialogOpen", true);
    pressKey(fixture.view.get(), Qt::Key_Right);
    pressKey(fixture.view.get(), Qt::Key_Down);
    QCOMPARE(fixture.root->property("panCount").toInt(), 0);
}

void TestImageShortcuts::shiftArrowsMoveOnePageInTwoPageModeLeftToRight()
{
    ImageShortcutsFixture fixture = createComicBookFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    prepareTwoPageSpread(fixture);

    pressKey(fixture.view.get(), Qt::Key_Right, Qt::ShiftModifier);
    QTRY_COMPARE(fixture.root->property("currentPageNumber").toInt(), 3);
    QTRY_COMPARE(fixture.root->property("currentLastPageNumber").toInt(), 4);

    pressKey(fixture.view.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTRY_COMPARE(fixture.root->property("currentPageNumber").toInt(), 2);
    QTRY_COMPARE(fixture.root->property("currentLastPageNumber").toInt(), 3);
}

void TestImageShortcuts::shiftArrowsMoveOnePageInTwoPageModeRightToLeft()
{
    ImageShortcutsFixture fixture = createComicBookFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    prepareTwoPageSpread(fixture);
    QVERIFY(fixture.root->setProperty("rightToLeftReadingEnabled", true));

    pressKey(fixture.view.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTRY_COMPARE(fixture.root->property("currentPageNumber").toInt(), 3);
    QTRY_COMPARE(fixture.root->property("currentLastPageNumber").toInt(), 4);

    pressKey(fixture.view.get(), Qt::Key_Right, Qt::ShiftModifier);
    QTRY_COMPARE(fixture.root->property("currentPageNumber").toInt(), 2);
    QTRY_COMPARE(fixture.root->property("currentLastPageNumber").toInt(), 3);
}

void TestImageShortcuts::shiftArrowsAreIgnoredWhileViewerShortcutsAreSuppressed()
{
    ImageShortcutsFixture fixture = createComicBookFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    prepareTwoPageSpread(fixture);

    fixture.root->setProperty("toolbarTextInputFocused", true);
    pressKey(fixture.view.get(), Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(fixture.root->property("currentPageNumber").toInt(), 2);

    fixture.root->setProperty("toolbarTextInputFocused", false);
    fixture.root->setProperty("helpDialogOpen", true);
    pressKey(fixture.view.get(), Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(fixture.root->property("currentPageNumber").toInt(), 2);
}

void TestImageShortcuts::configuredActionShortcutsTriggerActions()
{
    ImageShortcutsFixture fixture = createReadyFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(documentReady(fixture.root));

    QAction *rotateAction = fixture.application->action(QStringLiteral("view_rotate_clockwise"));
    QVERIFY(rotateAction != nullptr);
    QSignalSpy triggeredSpy(rotateAction, &QAction::triggered);

    pressKey(fixture.view.get(), Qt::Key_R, Qt::ControlModifier);
    QTRY_COMPARE(triggeredSpy.count(), 1);

    pressKey(fixture.view.get(), Qt::Key_R);
    QTRY_COMPARE(triggeredSpy.count(), 2);

    fixture.root->setProperty("toolbarTextInputFocused", true);
    pressKey(fixture.view.get(), Qt::Key_R);
    QCOMPARE(triggeredSpy.count(), 2);

    pressKey(fixture.view.get(), Qt::Key_R, Qt::ControlModifier);
    QTRY_COMPARE(triggeredSpy.count(), 3);
}

void TestImageShortcuts::videoViewerAliasTriggersFullscreenAction()
{
    ImageShortcutsFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY(fixture.root->setProperty("videoMode", true));

    QAction *fullscreenAction = fixture.application->action(QStringLiteral("window_fullscreen"));
    QVERIFY(fullscreenAction != nullptr);
    QSignalSpy triggeredSpy(fullscreenAction, &QAction::triggered);

    pressKey(fixture.view.get(), Qt::Key_F);

    QTRY_COMPARE(triggeredSpy.count(), 1);
    QCOMPARE(fixture.root->property("unsupportedVideoActionCount").toInt(), 0);
}

void TestImageShortcuts::videoImageOnlyShortcutsShowUnsupportedToast()
{
    ImageShortcutsFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY(fixture.root->setProperty("videoMode", true));

    QAction *rotateAction = fixture.application->action(QStringLiteral("view_rotate_clockwise"));
    QAction *zoomInAction = fixture.application->action(QStringLiteral("view_zoom_in"));
    QAction *fitAction = fixture.application->action(QStringLiteral("view_fit"));
    QVERIFY(rotateAction != nullptr);
    QVERIFY(zoomInAction != nullptr);
    QVERIFY(fitAction != nullptr);

    QSignalSpy rotateSpy(rotateAction, &QAction::triggered);
    QSignalSpy zoomInSpy(zoomInAction, &QAction::triggered);
    QSignalSpy fitSpy(fitAction, &QAction::triggered);

    pressKey(fixture.view.get(), Qt::Key_R, Qt::ControlModifier);
    QTRY_COMPARE(fixture.root->property("unsupportedVideoActionCount").toInt(), 1);
    QCOMPARE(rotateSpy.count(), 0);

    pressKey(fixture.view.get(), Qt::Key_R);
    QTRY_COMPARE(fixture.root->property("unsupportedVideoActionCount").toInt(), 2);
    QCOMPARE(rotateSpy.count(), 0);

    pressKey(fixture.view.get(), Qt::Key_Equal, Qt::ControlModifier);
    QTRY_COMPARE(fixture.root->property("unsupportedVideoActionCount").toInt(), 3);
    QCOMPARE(zoomInSpy.count(), 0);

    pressKey(fixture.view.get(), Qt::Key_1, Qt::ControlModifier);
    QTRY_COMPARE(fixture.root->property("unsupportedVideoActionCount").toInt(), 4);
    QCOMPARE(fitSpy.count(), 0);
}

QTEST_MAIN(TestImageShortcuts)

#include "test_imageshortcuts.moc"
