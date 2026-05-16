// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedocument.h"
#include "kiriviewapplication.h"
#include "kiriviewstate.h"
#include "localization.h"

#include <KConfigGroup>
#include <KLocalizedQmlContext>
#include <KSharedConfig>
#include <QAction>
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
    void ctrlMTogglesMenubarThroughAction();
    void ctrlMIgnoredWhileHelpDialogOpen();
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

    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
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
    property int panCount: 0
    property real lastPanX: 0
    property real lastPanY: 0

    function menuPresentation() {
        return application.menuPresentation;
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

    KiriImageDocument {
        id: imageDocument

        sourceUrl: "%2"
    }

    QtObject {
        id: imageViewport

        property bool imageHorizontallyPannable: root.imageHorizontallyPannable
        property bool imagePannable: root.imagePannable
        property real viewportHeight: 100
        property real viewportWidth: 100

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
    }

    QtObject {
        id: imageToolBar

        function textInputFocused() {
            return root.toolbarTextInputFocused;
        }
    }

    KiriViewQml.ImageShortcuts {
        application: application
        helpDialogOpen: root.helpDialogOpen
        imageDocument: imageDocument
        imageToolBar: imageToolBar
        imageViewport: imageViewport
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

int menuPresentation(QObject *root)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        root, "menuPresentation", Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked ? result.toInt() : -1;
}

bool documentReady(QObject *root)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        root, "documentReady", Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked && result.toBool();
}

void pressCtrlM(QQuickView *view)
{
    QTest::keyClick(view, Qt::Key_M, Qt::ControlModifier);
    QCoreApplication::processEvents();
}

void pressKey(QQuickView *view, Qt::Key key)
{
    QTest::keyClick(view, key);
    QCoreApplication::processEvents();
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

    QAction *previousAction = fixture.application->action(QStringLiteral("go_previous_image"));
    QAction *nextAction = fixture.application->action(QStringLiteral("go_next_image"));
    QVERIFY(previousAction != nullptr);
    QVERIFY(nextAction != nullptr);
    QSignalSpy previousSpy(previousAction, &QAction::triggered);
    QSignalSpy nextSpy(nextAction, &QAction::triggered);

    pressKey(fixture.view.get(), Qt::Key_Left);
    QCOMPARE(previousSpy.count(), 1);
    QCOMPARE(nextSpy.count(), 0);

    pressKey(fixture.view.get(), Qt::Key_Right);
    QCOMPARE(previousSpy.count(), 1);
    QCOMPARE(nextSpy.count(), 1);
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

void TestImageShortcuts::ctrlMTogglesMenubarThroughAction()
{
    ImageShortcutsFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QAction *showMenubarAction
        = fixture.application->action(QStringLiteral("options_show_menubar"));
    QVERIFY(showMenubarAction != nullptr);
    QSignalSpy triggeredSpy(showMenubarAction, &QAction::triggered);

    QCOMPARE(menuPresentation(fixture.root), static_cast<int>(KiriViewApplication::HamburgerMenu));
    QVERIFY(!showMenubarAction->isChecked());

    pressCtrlM(fixture.view.get());

    QTRY_COMPARE(triggeredSpy.count(), 1);
    QCOMPARE(menuPresentation(fixture.root), static_cast<int>(KiriViewApplication::MenuBar));
    QVERIFY(showMenubarAction->isChecked());

    pressCtrlM(fixture.view.get());

    QTRY_COMPARE(triggeredSpy.count(), 2);
    QCOMPARE(menuPresentation(fixture.root), static_cast<int>(KiriViewApplication::HamburgerMenu));
    QVERIFY(!showMenubarAction->isChecked());
}

void TestImageShortcuts::ctrlMIgnoredWhileHelpDialogOpen()
{
    ImageShortcutsFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QAction *showMenubarAction
        = fixture.application->action(QStringLiteral("options_show_menubar"));
    QVERIFY(showMenubarAction != nullptr);
    QSignalSpy triggeredSpy(showMenubarAction, &QAction::triggered);

    fixture.root->setProperty("helpDialogOpen", true);
    pressCtrlM(fixture.view.get());

    QCOMPARE(triggeredSpy.count(), 0);
    QCOMPARE(menuPresentation(fixture.root), static_cast<int>(KiriViewApplication::HamburgerMenu));
    QVERIFY(!showMenubarAction->isChecked());
}

QTEST_MAIN(TestImageShortcuts)

#include "test_imageshortcuts.moc"
