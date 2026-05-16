// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedocument.h"
#include "kiriviewapplication.h"
#include "localization.h"
#include "menuaccesskeyrouter.h"

#include <KLocalizedQmlContext>
#include <KZip>
#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>
#include <memory>

class TestToolBarApplicationMenu : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void buttonClickTogglesMenu();
    void outsideClickClosesMenu();
    void openApplicationMenuOnlyOpens();
    void mnemonicRoutingStillTriggersMenuAction();
    void toolbarActionOrderKeepsReadingDirectionBesideSpread();
    void pageNavigationButtonsUseSemanticActionsForReadingDirection();
    void menubarGoMenuOrderFollowsReadingDirection();
    void imageActionsApplicationMenuArchiveOrderFollowsReadingDirection();
};

namespace {
struct ToolBarMenuFixture {
    std::unique_ptr<QQuickView> view;
    std::unique_ptr<QTemporaryDir> temporaryDirectory;
    QObject *root = nullptr;
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr; }
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
    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<MenuAccessKeyRouter>("io.github.hnjae.kiriview", 1, 0, "MenuAccessKeyRouter");
    registered = true;
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

    for (int index = 1; index <= 3; ++index) {
        const QString path
            = directory->filePath(QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0')));
        if (!writeTestPng(path)) {
            *errorString = QStringLiteral("failed to write test image %1").arg(path);
            return nullptr;
        }
    }

    *sourcePath = directory->filePath(QStringLiteral("02.png"));
    return directory;
}

std::unique_ptr<QTemporaryDir> createComicBookArchive(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary archive directory was not created");
        return nullptr;
    }

    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
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
    if (!archive.writeFile(QStringLiteral("01.png"), imageData)) {
        *errorString = QStringLiteral("failed to write test archive entry");
        return nullptr;
    }
    archive.close();

    *sourcePath = archivePath;
    return directory;
}

QString fixtureQml(const QString &sourceUrl = QString(), bool navigationActionsEnabled = false)
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml
import org.kde.kirigami as Kirigami

Item {
    id: root

    width: 720
    height: 420

    property int openTriggerCount: 0
    property int previousTriggerCount: 0
    property int nextTriggerCount: 0
    property bool navigationActionsEnabled: %3
    property bool rightToLeftReadingActive: false

    function documentReady() {
        return imageDocument.status === KiriImageDocument.Ready;
    }

    function applicationMenuButton() {
        return toolbar.applicationMenuButtonAnchor;
    }

    function applicationMenuButtonCenter() {
        const button = applicationMenuButton();
        if (!button || button.width <= 0 || button.height <= 0) {
            return Qt.point(-1, -1);
        }

        return button.mapToItem(root, button.width / 2, button.height / 2);
    }

    function applicationMenuOpen() {
        return toolbar.applicationMenuOpen();
    }

    function hasApplicationMenuButton() {
        return applicationMenuButton() !== null;
    }

    function openApplicationMenu() {
        return toolbar.openApplicationMenu();
    }

    function outsideClickPoint() {
        return Qt.point(root.width / 2, root.height - 12);
    }

    function toolbarControlTexts() {
        return toolbar.toolbarControls.map(action => action.text);
    }

    function resetNavigationTriggerCounts() {
        previousTriggerCount = 0;
        nextTriggerCount = 0;
    }

    KiriImageDocument {
        id: imageDocument

        sourceUrl: "%2"
    }

    Kirigami.Action {
        id: previousImageKirigamiAction

        enabled: root.navigationActionsEnabled
        icon.name: "go-previous-symbolic"
        text: "Previous Image"

        onTriggered: root.previousTriggerCount += 1
    }

    Kirigami.Action {
        id: nextImageKirigamiAction

        enabled: root.navigationActionsEnabled
        icon.name: "go-next-symbolic"
        text: "Next Image"

        onTriggered: root.nextTriggerCount += 1
    }

    Kirigami.Action {
        id: twoPageModeKirigamiAction

        enabled: false
        icon.name: "view-split-left-right-symbolic"
        text: "Two-Page Spread"
    }

    Kirigami.Action {
        id: rightToLeftReadingKirigamiAction

        enabled: false
        icon.name: "format-text-direction-rtl-symbolic"
        text: "Right-to-Left Reading"
    }

    Kirigami.Action {
        id: fitKirigamiAction

        enabled: false
        icon.name: "zoom-fit-best-symbolic"
        text: "Fit"
    }

    Kirigami.Action {
        id: fitHeightKirigamiAction

        enabled: false
        text: "Fit Height"
    }

    Kirigami.Action {
        id: fitWidthKirigamiAction

        enabled: false
        text: "Fit Width"
    }

    Kirigami.Action {
        id: openMenuAction

        text: "&Open"

        onTriggered: root.openTriggerCount += 1
    }

    Kirigami.Action {
        id: separatorAction

        separator: true
    }

    Kirigami.Action {
        id: quitMenuAction

        text: "&Quit"
    }

    QtObject {
        id: toolbarActions

        readonly property var previousImageAction: previousImageKirigamiAction
        readonly property var nextImageAction: nextImageKirigamiAction
        readonly property var rightToLeftReadingAction: rightToLeftReadingKirigamiAction
        readonly property var twoPageModeAction: twoPageModeKirigamiAction
        readonly property var fitAction: fitKirigamiAction
        readonly property var fitHeightAction: fitHeightKirigamiAction
        readonly property var fitWidthAction: fitWidthKirigamiAction
    }

    KiriViewQml.ImageToolBar {
        id: toolbar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        actions: toolbarActions
        applicationMenuActions: [openMenuAction, separatorAction, quitMenuAction]
        compact: true
        imageDocument: imageDocument
        imageReady: imageDocument.status === KiriImageDocument.Ready
        maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
        minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
        rightToLeftReadingActive: root.rightToLeftReadingActive
        showApplicationMenuActions: true
        zoomStepFactor: imageDocument.zoomStepFactor
    }
}
)")
        .arg(qmlSourceImport(), sourceUrl,
            navigationActionsEnabled ? QStringLiteral("true") : QStringLiteral("false"));
}

QString menuBarFixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml
import org.kde.kirigami as Kirigami

Item {
    id: root

    width: 720
    height: 420

    property bool rightToLeftReadingActive: false

    function sanitizedText(text) {
        return String(text).replace(/&/g, "");
    }

    function menuActionTexts(menu) {
        const texts = [];
        for (let index = 0; index < menu.count; ++index) {
            const item = menu.itemAt(index);
            const text = item && typeof item.text === "string" ? item.text : "";
            if (text.length > 0) {
                texts.push(sanitizedText(text));
            }
        }
        return texts;
    }

    function goMenuActionTexts() {
        return menuActionTexts(menuBar.menuAt(1));
    }

    KiriImageDocument {
        id: imageDocument
    }

    Kirigami.Action { id: stubOpenMenuAction; text: "Open" }
    Kirigami.Action { id: stubMoveToTrashMenuAction; text: "Move to Trash" }
    Kirigami.Action { id: stubDeleteFileMenuAction; text: "Delete Permanently" }
    Kirigami.Action { id: stubQuitMenuAction; text: "Quit" }
    Kirigami.Action { id: stubPreviousImageMenuAction; text: "Previous" }
    Kirigami.Action { id: stubNextImageMenuAction; text: "Next" }
    Kirigami.Action { id: stubFirstImageMenuAction; text: "First Image" }
    Kirigami.Action { id: stubLastImageMenuAction; text: "Last Image" }
    Kirigami.Action { id: stubPreviousContainerMenuAction; text: "Previous Archive" }
    Kirigami.Action { id: stubNextContainerMenuAction; text: "Next Archive" }
    Kirigami.Action { id: stubZoomInMenuAction; text: "Zoom In" }
    Kirigami.Action { id: stubZoomOutMenuAction; text: "Zoom Out" }
    Kirigami.Action { id: stubFitMenuAction; text: "Fit" }
    Kirigami.Action { id: stubFitHeightMenuAction; text: "Fit Height" }
    Kirigami.Action { id: stubFitWidthMenuAction; text: "Fit Width" }
    Kirigami.Action { id: stubActualSizeMenuAction; text: "Actual Size" }
    Kirigami.Action { id: stubRotateClockwiseMenuAction; text: "Rotate Clockwise" }
    Kirigami.Action { id: stubRotateCounterclockwiseMenuAction; text: "Rotate Counterclockwise" }
    Kirigami.Action { id: stubTwoPageModeMenuAction; text: "Two-Page Spread" }
    Kirigami.Action { id: stubRightToLeftReadingMenuAction; text: "Right-to-Left Reading" }
    Kirigami.Action { id: stubFullscreenMenuAction; text: "Fullscreen" }
    Kirigami.Action { id: stubShowMenubarMenuAction; text: "Show Menubar" }
    Kirigami.Action { id: stubConfigureShortcutsMenuAction; text: "Configure Shortcuts" }
    Kirigami.Action { id: stubShortcutHelpMenuAction; text: "Keyboard Shortcuts" }

    QtObject {
        id: menuActions

        readonly property var openMenuAction: stubOpenMenuAction
        readonly property var moveToTrashMenuAction: stubMoveToTrashMenuAction
        readonly property var deleteFileMenuAction: stubDeleteFileMenuAction
        readonly property var quitMenuAction: stubQuitMenuAction
        readonly property var previousImageMenuAction: stubPreviousImageMenuAction
        readonly property var nextImageMenuAction: stubNextImageMenuAction
        readonly property var firstImageMenuAction: stubFirstImageMenuAction
        readonly property var lastImageMenuAction: stubLastImageMenuAction
        readonly property var previousContainerMenuAction: stubPreviousContainerMenuAction
        readonly property var nextContainerMenuAction: stubNextContainerMenuAction
        readonly property var zoomInMenuAction: stubZoomInMenuAction
        readonly property var zoomOutMenuAction: stubZoomOutMenuAction
        readonly property var fitMenuAction: stubFitMenuAction
        readonly property var fitHeightMenuAction: stubFitHeightMenuAction
        readonly property var fitWidthMenuAction: stubFitWidthMenuAction
        readonly property var actualSizeMenuAction: stubActualSizeMenuAction
        readonly property var rotateClockwiseMenuAction: stubRotateClockwiseMenuAction
        readonly property var rotateCounterclockwiseMenuAction: stubRotateCounterclockwiseMenuAction
        readonly property var twoPageModeMenuAction: stubTwoPageModeMenuAction
        readonly property var rightToLeftReadingMenuAction: stubRightToLeftReadingMenuAction
        readonly property var fullscreenMenuAction: stubFullscreenMenuAction
        readonly property var showMenubarMenuAction: stubShowMenubarMenuAction
        readonly property var configureShortcutsMenuAction: stubConfigureShortcutsMenuAction
        readonly property var shortcutHelpMenuAction: stubShortcutHelpMenuAction
    }

    KiriViewQml.ApplicationMenuBar {
        id: menuBar

        actions: menuActions
        fullscreen: false
        imageDocument: imageDocument
        rightToLeftReadingActive: root.rightToLeftReadingActive
    }
}
)")
        .arg(qmlSourceImport());
}

QString imageActionsFixtureQml(const QString &sourceUrl)
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml

Item {
    id: root

    width: 720
    height: 420

    property alias rightToLeftReadingEnabled: imageDocument.rightToLeftReadingEnabled

    function applicationMenuActionTexts() {
        const texts = [];
        for (const action of imageActions.applicationMenuActions) {
            if (!action.separator) {
                texts.push(String(action.text).replace(/&/g, ""));
            }
        }
        return texts;
    }

    function documentReady() {
        return imageDocument.status === KiriImageDocument.Ready;
    }

    function rightToLeftReadingActive() {
        return imageActions.rightToLeftReadingActive;
    }

    function rightToLeftReadingAvailable() {
        return imageDocument.rightToLeftReadingAvailable;
    }

    KiriViewApplication {
        id: application
    }

    KiriImageDocument {
        id: imageDocument

        sourceUrl: "%2"
    }

    KiriViewQml.ImageActions {
        id: imageActions

        application: application
        fullscreen: false
        helpDialogOpen: false
        imageDocument: imageDocument
        imageReady: imageDocument.status === KiriImageDocument.Ready
    }
}
)")
        .arg(qmlSourceImport(), sourceUrl);
}

ToolBarMenuFixture createFixtureFromQml(const QString &qml, const QUrl &componentUrl)
{
    ToolBarMenuFixture fixture;
    registerKiriViewQmlTypes();
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(720, 420);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());
    KLocalization::setupLocalizedContext(fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(qml.toUtf8(), componentUrl);
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

    fixture.view->setContent(componentUrl, &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    return fixture;
}

ToolBarMenuFixture createFixture(
    const QString &sourceUrl = QString(), bool navigationActionsEnabled = false)
{
    return createFixtureFromQml(fixtureQml(sourceUrl, navigationActionsEnabled),
        QUrl(QStringLiteral("memory:test_toolbarapplicationmenu.qml")));
}

ToolBarMenuFixture createMenuBarFixture()
{
    return createFixtureFromQml(
        menuBarFixtureQml(), QUrl(QStringLiteral("memory:test_applicationmenubar.qml")));
}

ToolBarMenuFixture createImageActionsFixture(const QString &sourceUrl)
{
    return createFixtureFromQml(imageActionsFixtureQml(sourceUrl),
        QUrl(QStringLiteral("memory:test_imageactions_applicationmenu.qml")));
}

QVariant invokeVariant(QObject *root, const char *method, bool *invoked = nullptr)
{
    QVariant result;
    const bool ok = QMetaObject::invokeMethod(
        root, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    if (invoked != nullptr) {
        *invoked = ok;
    }
    return result;
}

bool invokeBool(QObject *root, const char *method, bool *invoked = nullptr)
{
    bool ok = false;
    const QVariant result = invokeVariant(root, method, &ok);
    if (invoked != nullptr) {
        *invoked = ok;
    }
    return ok && result.toBool();
}

bool applicationMenuOpen(QObject *root) { return invokeBool(root, "applicationMenuOpen"); }

QStringList invokeStringList(QObject *root, const char *method, bool *ok = nullptr)
{
    bool invoked = false;
    const QVariant result = invokeVariant(root, method, &invoked);
    QStringList strings;
    if (result.canConvert<QStringList>()) {
        strings = result.toStringList();
        if (ok != nullptr) {
            *ok = invoked;
        }
        return strings;
    }

    const QVariantList values = result.toList();
    for (const QVariant &value : values) {
        strings.append(value.toString());
    }
    if (ok != nullptr) {
        *ok = invoked;
    }
    return strings;
}

QPoint invokePoint(QObject *root, const char *method, bool *ok = nullptr)
{
    bool invoked = false;
    const QVariant result = invokeVariant(root, method, &invoked);
    const bool converted = result.canConvert<QPointF>();
    if (ok != nullptr) {
        *ok = invoked && converted;
    }
    if (!invoked || !converted) {
        return QPoint(-1, -1);
    }

    return result.toPointF().toPoint();
}

QQuickItem *findQuickItem(QObject *root, const QString &objectName)
{
    return root->findChild<QQuickItem *>(objectName, Qt::FindChildrenRecursively);
}

QPoint quickItemCenter(QObject *root, QQuickItem *item)
{
    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root);
    if (rootItem == nullptr || item == nullptr || item->width() <= 0 || item->height() <= 0) {
        return QPoint(-1, -1);
    }

    return item->mapToItem(rootItem, item->width() / 2, item->height() / 2).toPoint();
}

void clickAt(QQuickView *view, const QPoint &point)
{
    QVERIFY(point.x() >= 0);
    QVERIFY(point.y() >= 0);

    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, point);
    QCoreApplication::processEvents();
}

void clickItem(const ToolBarMenuFixture &fixture, QQuickItem *item)
{
    clickAt(fixture.view.get(), quickItemCenter(fixture.root, item));
}

void clickApplicationMenuButton(const ToolBarMenuFixture &fixture)
{
    bool ok = false;
    const QPoint point = invokePoint(fixture.root, "applicationMenuButtonCenter", &ok);
    QVERIFY(ok);
    clickAt(fixture.view.get(), point);
}

void clickOutsideMenuParent(const ToolBarMenuFixture &fixture)
{
    bool ok = false;
    const QPoint point = invokePoint(fixture.root, "outsideClickPoint", &ok);
    QVERIFY(ok);
    clickAt(fixture.view.get(), point);
}
}

void TestToolBarApplicationMenu::init()
{
    QTest::failOnWarning(QRegularExpression(
        QStringLiteral(".*Created graphical object was not placed in the graphics scene.*")));
}

void TestToolBarApplicationMenu::buttonClickTogglesMenu()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "hasApplicationMenuButton"));

    bool ok = false;
    const QPoint buttonPoint = invokePoint(fixture.root, "applicationMenuButtonCenter", &ok);
    QVERIFY(ok);

    clickAt(fixture.view.get(), buttonPoint);
    QTRY_VERIFY(applicationMenuOpen(fixture.root));

    clickAt(fixture.view.get(), buttonPoint);
    QTRY_VERIFY(!applicationMenuOpen(fixture.root));
    QTest::qWait(50);
    QVERIFY(!applicationMenuOpen(fixture.root));
}

void TestToolBarApplicationMenu::outsideClickClosesMenu()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "hasApplicationMenuButton"));

    clickApplicationMenuButton(fixture);
    QTRY_VERIFY(applicationMenuOpen(fixture.root));

    clickOutsideMenuParent(fixture);
    QTRY_VERIFY(!applicationMenuOpen(fixture.root));
}

void TestToolBarApplicationMenu::openApplicationMenuOnlyOpens()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    bool invoked = false;
    QVERIFY(invokeBool(fixture.root, "openApplicationMenu", &invoked));
    QVERIFY(invoked);
    QTRY_VERIFY(applicationMenuOpen(fixture.root));

    QVERIFY(invokeBool(fixture.root, "openApplicationMenu", &invoked));
    QVERIFY(invoked);
    QTest::qWait(50);
    QVERIFY(applicationMenuOpen(fixture.root));
}

void TestToolBarApplicationMenu::mnemonicRoutingStillTriggersMenuAction()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "hasApplicationMenuButton"));

    clickApplicationMenuButton(fixture);
    QTRY_VERIFY(applicationMenuOpen(fixture.root));

    QTest::keyClick(fixture.view.get(), Qt::Key_O, Qt::AltModifier);
    QCoreApplication::processEvents();

    QTRY_COMPARE(fixture.root->property("openTriggerCount").toInt(), 1);
}

void TestToolBarApplicationMenu::toolbarActionOrderKeepsReadingDirectionBesideSpread()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts,
        QStringList({ QStringLiteral("Right-to-Left Reading"), QStringLiteral("Two-Page Spread"),
            QStringLiteral("Zoom"), QStringLiteral("Fit") }));
}

void TestToolBarApplicationMenu::pageNavigationButtonsUseSemanticActionsForReadingDirection()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> imageDirectory = createImageDirectory(&sourcePath, &errorString);
    QVERIFY2(imageDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture = createFixture(QUrl::fromLocalFile(sourcePath).toString(), true);
    fixture.temporaryDirectory = std::move(imageDirectory);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    QQuickItem *leftButton
        = findQuickItem(fixture.root, QStringLiteral("leftPageNavigationButton"));
    QQuickItem *rightButton
        = findQuickItem(fixture.root, QStringLiteral("rightPageNavigationButton"));
    QVERIFY2(leftButton != nullptr, "left page navigation button was not created");
    QVERIFY2(rightButton != nullptr, "right page navigation button was not created");
    QTRY_VERIFY(leftButton->isEnabled());
    QTRY_VERIFY(rightButton->isEnabled());

    QCOMPARE(leftButton->property("text").toString(), QStringLiteral("Previous Image"));
    QCOMPARE(leftButton->property("toolTipText").toString(), QStringLiteral("Previous Image"));
    QCOMPARE(rightButton->property("text").toString(), QStringLiteral("Next Image"));
    QCOMPARE(rightButton->property("toolTipText").toString(), QStringLiteral("Next Image"));

    clickItem(fixture, leftButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 0);

    clickItem(fixture, rightButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 1);

    QVERIFY(QMetaObject::invokeMethod(
        fixture.root, "resetNavigationTriggerCounts", Qt::DirectConnection));
    fixture.root->setProperty("rightToLeftReadingActive", true);
    QCoreApplication::processEvents();

    QTRY_COMPARE(leftButton->property("text").toString(), QStringLiteral("Next Image"));
    QCOMPARE(leftButton->property("toolTipText").toString(), QStringLiteral("Next Image"));
    QTRY_COMPARE(rightButton->property("text").toString(), QStringLiteral("Previous Image"));
    QCOMPARE(rightButton->property("toolTipText").toString(), QStringLiteral("Previous Image"));

    clickItem(fixture, leftButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 0);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 1);

    clickItem(fixture, rightButton);
    QCOMPARE(fixture.root->property("previousTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("nextTriggerCount").toInt(), 1);
}

void TestToolBarApplicationMenu::menubarGoMenuOrderFollowsReadingDirection()
{
    ToolBarMenuFixture fixture = createMenuBarFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    const QStringList leftToRightOrder({
        QStringLiteral("Previous"),
        QStringLiteral("Next"),
        QStringLiteral("First Image"),
        QStringLiteral("Last Image"),
        QStringLiteral("Previous Archive"),
        QStringLiteral("Next Archive"),
    });
    const QStringList rightToLeftOrder({
        QStringLiteral("Next"),
        QStringLiteral("Previous"),
        QStringLiteral("First Image"),
        QStringLiteral("Last Image"),
        QStringLiteral("Next Archive"),
        QStringLiteral("Previous Archive"),
    });

    bool ok = false;
    QCOMPARE(invokeStringList(fixture.root, "goMenuActionTexts", &ok), leftToRightOrder);
    QVERIFY(ok);

    fixture.root->setProperty("rightToLeftReadingActive", true);
    QCoreApplication::processEvents();

    QTRY_COMPARE(invokeStringList(fixture.root, "goMenuActionTexts"), rightToLeftOrder);
}

void TestToolBarApplicationMenu::imageActionsApplicationMenuArchiveOrderFollowsReadingDirection()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> archiveDirectory
        = createComicBookArchive(&sourcePath, &errorString);
    QVERIFY2(archiveDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture
        = createImageActionsFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(archiveDirectory);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    QTRY_VERIFY(invokeBool(fixture.root, "rightToLeftReadingAvailable"));

    QStringList texts = invokeStringList(fixture.root, "applicationMenuActionTexts");
    const int previousArchiveIndex = texts.indexOf(QStringLiteral("Previous Archive"));
    const int nextArchiveIndex = texts.indexOf(QStringLiteral("Next Archive"));
    QVERIFY(previousArchiveIndex >= 0);
    QVERIFY(nextArchiveIndex >= 0);
    QVERIFY(previousArchiveIndex < nextArchiveIndex);

    fixture.root->setProperty("rightToLeftReadingEnabled", true);
    QCoreApplication::processEvents();
    QTRY_VERIFY(invokeBool(fixture.root, "rightToLeftReadingActive"));

    texts = invokeStringList(fixture.root, "applicationMenuActionTexts");
    const int rtlPreviousArchiveIndex = texts.indexOf(QStringLiteral("Previous Archive"));
    const int rtlNextArchiveIndex = texts.indexOf(QStringLiteral("Next Archive"));
    QVERIFY(rtlPreviousArchiveIndex >= 0);
    QVERIFY(rtlNextArchiveIndex >= 0);
    QVERIFY(rtlNextArchiveIndex < rtlPreviousArchiveIndex);
}

QTEST_MAIN(TestToolBarApplicationMenu)

#include "test_toolbarapplicationmenu.moc"
