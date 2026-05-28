// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/imageactionavailability.h"
#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "facade/kiriviewapplication.h"
#include "facade/menuaccesskeyrouter.h"
#include "localization/localization.h"

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
#include <QVariantMap>
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
    void toolbarReadingControlsUseTextButtons();
    void toolbarReadingControlMnemonicsTriggerActions();
    void fitSplitButtonPrimaryTriggersSelectedMode();
    void fitSplitButtonMenuSelectionUpdatesPrimaryMode();
    void fitSplitButtonKeepsLastFitSelectionAfterManualZoom();
    void fitSplitButtonCollapsesLabelWhenConstrained();
    void toolbarActionOrderKeepsReadingDirectionBesideSpread();
    void emptyToolbarHidesReadingControls();
    void directImageToolbarHidesReadingControls();
    void videoToolbarHidesReadingControlsAndDisablesImageControls();
    void comicArchiveToolbarShowsEnabledReadingControls();
    void generalArchiveToolbarShowsDisabledReadingControls();
    void directoryCollectionToolbarShowsDisabledReadingControls();
    void pageNavigationButtonsUseSemanticActionsForReadingDirection();
    void menubarGoMenuOrderFollowsReadingDirection();
    void menubarGoMenuIconsFollowReadingDirection();
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
    qmlRegisterType<ImageActionAvailability>(
        "io.github.hnjae.kiriview", 1, 0, "ImageActionAvailability");
    qmlRegisterType<KiriDocumentSession>("io.github.hnjae.kiriview", 1, 0, "KiriDocumentSession");
    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<KiriVideoDocument>("io.github.hnjae.kiriview", 1, 0, "KiriVideoDocument");
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

std::unique_ptr<QTemporaryDir> createDirectoryCollection(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary directory collection was not created");
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

std::unique_ptr<QTemporaryDir> createZipArchive(QString *sourcePath, QString *errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary archive directory was not created");
        return nullptr;
    }

    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::green);
    QByteArray imageData;
    QBuffer imageBuffer(&imageData);
    if (!imageBuffer.open(QIODevice::WriteOnly) || !image.save(&imageBuffer, "PNG")) {
        *errorString = QStringLiteral("failed to encode test archive image");
        return nullptr;
    }

    const QString archivePath = directory->filePath(QStringLiteral("book.zip"));
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

QString fixtureQml(const QString &sourceUrl = QString(), bool navigationActionsEnabled = false,
    const QString &readingControlsVisibleExpression = QStringLiteral("true"),
    const QString &readingControlsEnabledExpression = QStringLiteral("!root.videoMode"))
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls
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
    property int rightToLeftTriggerCount: 0
    property int twoPageTriggerCount: 0
    property int fitTriggerCount: 0
    property int fitHeightTriggerCount: 0
    property int fitWidthTriggerCount: 0
    property bool navigationActionsEnabled: %3
    property bool rightToLeftReadingActive: false
    property bool videoMode: false
    readonly property bool readingControlsVisible: %4

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

    function sanitizedText(text) {
        return String(text).replace(/&/g, "");
    }

    function toolbarControlTexts() {
        return toolbar.toolbarControls.map(action => sanitizedText(action.text));
    }

    function toolbarControlEnabledStates() {
        return toolbar.toolbarControls.map(action => action.enabled);
    }

    function toolbarButtonForAction(action) {
        let fallback = null;
        function visit(item) {
            if (!item) {
                return null;
            }
            if ("action" in item && item.action === action) {
                if (item.visible && item.width > 0 && item.height > 0) {
                    return item;
                }
                fallback = fallback ?? item;
            }
            const children = item.children ?? [];
            for (let index = 0; index < children.length; ++index) {
                const found = visit(children[index]);
                if (found !== null) {
                    return found;
                }
            }
            return null;
        }
        return visit(toolbar) ?? fallback;
    }

    function toolbarButtonStateForAction(action) {
        const button = toolbarButtonForAction(action);
        return {
            found: button !== null,
            iconName: button !== null ? ("icon" in button ? button.icon.name : (button.iconName ?? "")) : "",
            iconOnly: button !== null && button.display === Controls.AbstractButton.IconOnly,
            label: button !== null ? sanitizedText(button.text) : "",
            text: button !== null ? button.text : "",
            textBesideIcon: button !== null && button.display === Controls.AbstractButton.TextBesideIcon,
            tooltip: action.tooltip,
            visible: button !== null && button.visible && button.width > 0 && button.height > 0,
        };
    }

    function rightToLeftToolbarButtonState() {
        return toolbarButtonStateForAction(rightToLeftReadingKirigamiAction);
    }

    function twoPageToolbarButtonState() {
        return toolbarButtonStateForAction(twoPageModeKirigamiAction);
    }

    function fitToolbarButtonState() {
        return toolbarButtonStateForAction(toolbar.fitMenuAction);
    }

    function selectedFitMode() {
        return toolbar.selectedFitMode;
    }

    function setSelectedFitModeToFitWidth() {
        toolbar.selectedFitMode = KiriImageDocument.FitWidth;
    }

    function setImageFitWidth() {
        imageDocument.setFitMode(KiriImageDocument.FitWidth);
    }

    function setImageManualZoom() {
        imageDocument.zoomPercent = 125;
    }

    function setToolbarWidth(width) {
        root.width = width;
        toolbar.width = width;
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

        enabled: %5
        icon.name: "view-split-left-right-symbolic"
        text: "Two-Page &Spread"
        tooltip: "Two-Page Spread"

        onTriggered: root.twoPageTriggerCount += 1
    }

    Kirigami.Action {
        id: rightToLeftReadingKirigamiAction

        enabled: %5
        icon.name: "format-text-direction-rtl-symbolic"
        text: "&Right-to-Left"
        tooltip: "Right-to-Left Reading"

        onTriggered: root.rightToLeftTriggerCount += 1
    }

    Kirigami.Action {
        id: fitKirigamiAction

        enabled: !root.videoMode
        icon.name: "zoom-fit-best-symbolic"
        text: "Fit"

        onTriggered: root.fitTriggerCount += 1
    }

    Kirigami.Action {
        id: fitHeightKirigamiAction

        enabled: !root.videoMode
        text: "Fit Height"

        onTriggered: root.fitHeightTriggerCount += 1
    }

    Kirigami.Action {
        id: fitWidthKirigamiAction

        enabled: !root.videoMode
        text: "Fit Width"

        onTriggered: root.fitWidthTriggerCount += 1
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
        activeNavigationAvailable: imageDocument.pageCount > 0
        activeNavigationCount: imageDocument.pageCount
        activeNavigationCurrentNumber: imageDocument.currentPageNumber
        activeNavigationEditable: imageDocument.pageCount > 0
        activeNavigationKnown: imageDocument.currentPageNumber > 0 && imageDocument.pageCount > 0
        applicationMenuActions: [openMenuAction, separatorAction, quitMenuAction]
        compact: true
        imageDocument: imageDocument
        imageReady: !root.videoMode && imageDocument.status === KiriImageDocument.Ready
        maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
        minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
        rightToLeftReadingActive: root.rightToLeftReadingActive
        rightToLeftReadingControlVisible: root.readingControlsVisible
        showApplicationMenuActions: true
        twoPageModeControlVisible: root.readingControlsVisible
        videoMode: root.videoMode
        zoomEditable: !root.videoMode && imageDocument.zoomPercentKnown
        zoomPercent: root.videoMode ? 67 : (imageDocument.zoomPercentKnown ? imageDocument.zoomPercent : 0)
        zoomPercentAvailable: root.videoMode || imageDocument.zoomPercentKnown
        zoomPercentKnown: root.videoMode ? true : imageDocument.zoomPercentKnown
        zoomStepFactor: imageDocument.zoomStepFactor
    }
}
)")
        .arg(qmlSourceImport(), sourceUrl,
            navigationActionsEnabled ? QStringLiteral("true") : QStringLiteral("false"),
            readingControlsVisibleExpression, readingControlsEnabledExpression);
}

QString openedCollectionScopeFixtureQml(
    const QString &sourceUrl = QString(), bool navigationActionsEnabled = false)
{
    return fixtureQml(sourceUrl, navigationActionsEnabled,
        QStringLiteral("!root.videoMode && imageDocument.openedCollectionScopeActive"),
        QStringLiteral("!root.videoMode && imageDocument.rightToLeftReadingAvailable"));
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

    function menuActionIconNames(menu) {
        const iconNames = [];
        for (let index = 0; index < menu.count; ++index) {
            const item = menu.itemAt(index);
            const text = item && typeof item.text === "string" ? item.text : "";
            if (text.length > 0) {
                iconNames.push(item.icon.name);
            }
        }
        return iconNames;
    }

    function goMenuActionIconNames() {
        return menuActionIconNames(menuBar.menuAt(1));
    }

    KiriImageDocument {
        id: imageDocument
    }

    Kirigami.Action { id: stubOpenMenuAction; text: "Open" }
    Kirigami.Action { id: stubOpenWithMenuAction; text: "Open With..." }
    Kirigami.Action { id: stubMoveToTrashMenuAction; text: "Move to Trash" }
    Kirigami.Action { id: stubDeleteFileMenuAction; text: "Delete Permanently" }
    Kirigami.Action { id: stubQuitMenuAction; text: "Quit" }
    Kirigami.Action { id: stubPreviousImageMenuAction; icon.name: "go-previous"; text: "Previous" }
    Kirigami.Action { id: stubNextImageMenuAction; icon.name: "go-next"; text: "Next" }
    Kirigami.Action { id: stubFirstImageMenuAction; icon.name: "go-first-symbolic"; text: "First Image" }
    Kirigami.Action { id: stubLastImageMenuAction; icon.name: "go-last-symbolic"; text: "Last Image" }
    Kirigami.Action { id: stubPreviousContainerMenuAction; icon.name: "go-previous-use"; text: "Previous Archive" }
    Kirigami.Action { id: stubNextContainerMenuAction; icon.name: "go-next-use"; text: "Next Archive" }
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
        readonly property var openWithMenuAction: stubOpenWithMenuAction
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

    readonly property var imageDocument: documentSession.imageDocument
    property bool rightToLeftReadingEnabled: imageDocument.rightToLeftReadingEnabled

    onRightToLeftReadingEnabledChanged: {
        if (imageDocument.rightToLeftReadingEnabled !== rightToLeftReadingEnabled) {
            imageDocument.rightToLeftReadingEnabled = rightToLeftReadingEnabled;
        }
    }

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

    KiriDocumentSession {
        id: documentSession

        sourceUrl: "%2"
    }

    ImageActionAvailability {
        id: actionAvailability

        containerNavigationAvailable: imageDocument.containerNavigationAvailable
        fileDeletionInProgress: imageDocument.fileDeletionInProgress
        helpDialogOpen: false
        imageReady: imageDocument.status === KiriImageDocument.Ready
        rightToLeftReadingAvailable: imageDocument.rightToLeftReadingAvailable
        rightToLeftReadingEnabled: imageDocument.rightToLeftReadingEnabled
        twoPageModeAvailable: imageDocument.twoPageModeAvailable
        twoPageModeEnabled: imageDocument.twoPageModeEnabled
    }

    KiriViewQml.ImageActions {
        id: imageActions

        application: application
        actionAvailability: actionAvailability
        documentSession: documentSession
        fullscreen: false
        imageDocument: imageDocument
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

ToolBarMenuFixture createOpenedCollectionScopeFixture(
    const QString &sourceUrl = QString(), bool navigationActionsEnabled = false)
{
    return createFixtureFromQml(
        openedCollectionScopeFixtureQml(sourceUrl, navigationActionsEnabled),
        QUrl(QStringLiteral("memory:test_toolbar_opened_collection_scope.qml")));
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

QVariantMap invokeVariantMap(QObject *root, const char *method, bool *ok = nullptr)
{
    bool invoked = false;
    const QVariant result = invokeVariant(root, method, &invoked);
    const bool converted = result.canConvert<QVariantMap>();
    if (ok != nullptr) {
        *ok = invoked && converted;
    }
    return result.toMap();
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

int invokeInt(QObject *root, const char *method, bool *invoked = nullptr)
{
    bool ok = false;
    const QVariant result = invokeVariant(root, method, &ok);
    if (invoked != nullptr) {
        *invoked = ok;
    }
    return ok ? result.toInt() : 0;
}

void invokeVoid(QObject *root, const char *method)
{
    QVERIFY(QMetaObject::invokeMethod(root, method, Qt::DirectConnection));
}

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

void TestToolBarApplicationMenu::toolbarReadingControlsUseTextButtons()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    bool ok = false;
    const QVariantMap rightToLeftState
        = invokeVariantMap(fixture.root, "rightToLeftToolbarButtonState", &ok);
    QVERIFY(ok);
    QVERIFY(rightToLeftState.value(QStringLiteral("found")).toBool());
    QVERIFY(rightToLeftState.value(QStringLiteral("visible")).toBool());
    QVERIFY(rightToLeftState.value(QStringLiteral("textBesideIcon")).toBool());
    QCOMPARE(rightToLeftState.value(QStringLiteral("text")).toString(),
        QStringLiteral("&Right-to-Left"));
    QCOMPARE(rightToLeftState.value(QStringLiteral("label")).toString(),
        QStringLiteral("Right-to-Left"));
    QCOMPARE(rightToLeftState.value(QStringLiteral("tooltip")).toString(),
        QStringLiteral("Right-to-Left Reading"));

    const QVariantMap twoPageState
        = invokeVariantMap(fixture.root, "twoPageToolbarButtonState", &ok);
    QVERIFY(ok);
    QVERIFY(twoPageState.value(QStringLiteral("found")).toBool());
    QVERIFY(twoPageState.value(QStringLiteral("visible")).toBool());
    QVERIFY(twoPageState.value(QStringLiteral("textBesideIcon")).toBool());
    QCOMPARE(
        twoPageState.value(QStringLiteral("text")).toString(), QStringLiteral("Two-Page &Spread"));
    QCOMPARE(
        twoPageState.value(QStringLiteral("label")).toString(), QStringLiteral("Two-Page Spread"));
    QCOMPARE(twoPageState.value(QStringLiteral("tooltip")).toString(),
        QStringLiteral("Two-Page Spread"));

    const QVariantMap fitState = invokeVariantMap(fixture.root, "fitToolbarButtonState", &ok);
    QVERIFY(ok);
    QVERIFY(fitState.value(QStringLiteral("found")).toBool());
    QVERIFY(fitState.value(QStringLiteral("visible")).toBool());
    QVERIFY(!fitState.value(QStringLiteral("iconOnly")).toBool());
    QVERIFY(fitState.value(QStringLiteral("textBesideIcon")).toBool());
    QCOMPARE(fitState.value(QStringLiteral("label")).toString(), QStringLiteral("Fit to Window"));
    QCOMPARE(fitState.value(QStringLiteral("tooltip")).toString(), QStringLiteral("Fit to Window"));
}

void TestToolBarApplicationMenu::toolbarReadingControlMnemonicsTriggerActions()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    bool ok = false;
    QTRY_VERIFY(invokeVariantMap(fixture.root, "rightToLeftToolbarButtonState", &ok)
            .value(QStringLiteral("visible"))
            .toBool());
    QVERIFY(ok);
    QTRY_VERIFY(invokeVariantMap(fixture.root, "twoPageToolbarButtonState", &ok)
            .value(QStringLiteral("visible"))
            .toBool());
    QVERIFY(ok);

    QTest::keyClick(fixture.view.get(), Qt::Key_R, Qt::AltModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(fixture.root->property("rightToLeftTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("twoPageTriggerCount").toInt(), 0);

    QTest::keyClick(fixture.view.get(), Qt::Key_S, Qt::AltModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(fixture.root->property("twoPageTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("rightToLeftTriggerCount").toInt(), 1);
}

void TestToolBarApplicationMenu::fitSplitButtonPrimaryTriggersSelectedMode()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QQuickItem *primaryButton = findQuickItem(fixture.root, QStringLiteral("fitPrimaryButton"));
    QVERIFY2(primaryButton != nullptr, "fit primary button was not created");
    QTRY_VERIFY(primaryButton->isEnabled());

    clickItem(fixture, primaryButton);
    QCOMPARE(fixture.root->property("fitTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("fitWidthTriggerCount").toInt(), 0);
    QCOMPARE(fixture.root->property("fitHeightTriggerCount").toInt(), 0);

    invokeVoid(fixture.root, "setSelectedFitModeToFitWidth");
    QCoreApplication::processEvents();

    clickItem(fixture, primaryButton);
    QCOMPARE(fixture.root->property("fitTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("fitWidthTriggerCount").toInt(), 1);
    QCOMPARE(fixture.root->property("fitHeightTriggerCount").toInt(), 0);
}

void TestToolBarApplicationMenu::fitSplitButtonMenuSelectionUpdatesPrimaryMode()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QQuickItem *menuButton = findQuickItem(fixture.root, QStringLiteral("fitMenuButton"));
    QVERIFY2(menuButton != nullptr, "fit menu button was not created");
    QTRY_VERIFY(menuButton->isEnabled());

    clickItem(fixture, menuButton);
    QQuickItem *fitWidthMenuItem = findQuickItem(fixture.root, QStringLiteral("fitWidthMenuItem"));
    QVERIFY2(fitWidthMenuItem != nullptr, "fit width menu item was not created");
    QTRY_VERIFY(fitWidthMenuItem->isVisible());

    clickItem(fixture, fitWidthMenuItem);
    QTRY_COMPARE(fixture.root->property("fitWidthTriggerCount").toInt(), 1);
    QCOMPARE(invokeInt(fixture.root, "selectedFitMode"),
        static_cast<int>(KiriImageDocument::ZoomMode::FitWidth));

    bool ok = false;
    const QVariantMap fitState = invokeVariantMap(fixture.root, "fitToolbarButtonState", &ok);
    QVERIFY(ok);
    QCOMPARE(fitState.value(QStringLiteral("label")).toString(), QStringLiteral("Fit Width"));
    QCOMPARE(
        fitState.value(QStringLiteral("iconName")).toString(), QStringLiteral("zoom-fit-width"));
}

void TestToolBarApplicationMenu::fitSplitButtonKeepsLastFitSelectionAfterManualZoom()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> imageDirectory = createImageDirectory(&sourcePath, &errorString);
    QVERIFY2(imageDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture
        = createOpenedCollectionScopeFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(imageDirectory);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    invokeVoid(fixture.root, "setImageFitWidth");
    QTRY_COMPARE(invokeInt(fixture.root, "selectedFitMode"),
        static_cast<int>(KiriImageDocument::ZoomMode::FitWidth));

    invokeVoid(fixture.root, "setImageManualZoom");
    QCoreApplication::processEvents();

    QCOMPARE(invokeInt(fixture.root, "selectedFitMode"),
        static_cast<int>(KiriImageDocument::ZoomMode::FitWidth));
    bool ok = false;
    const QVariantMap fitState = invokeVariantMap(fixture.root, "fitToolbarButtonState", &ok);
    QVERIFY(ok);
    QCOMPARE(fitState.value(QStringLiteral("label")).toString(), QStringLiteral("Fit Width"));
}

void TestToolBarApplicationMenu::fitSplitButtonCollapsesLabelWhenConstrained()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    bool ok = false;
    QVariantMap fitState = invokeVariantMap(fixture.root, "fitToolbarButtonState", &ok);
    QVERIFY(ok);
    QVERIFY(fitState.value(QStringLiteral("textBesideIcon")).toBool());
    QCOMPARE(fitState.value(QStringLiteral("label")).toString(), QStringLiteral("Fit to Window"));

    fixture.view->resize(360, 420);
    QCoreApplication::processEvents();

    fitState = invokeVariantMap(fixture.root, "fitToolbarButtonState", &ok);
    QVERIFY(ok);
    QVERIFY(fitState.value(QStringLiteral("iconOnly")).toBool());
    QCOMPARE(fitState.value(QStringLiteral("label")).toString(), QStringLiteral("Fit to Window"));
}

void TestToolBarApplicationMenu::toolbarActionOrderKeepsReadingDirectionBesideSpread()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts,
        QStringList({ QStringLiteral("Right-to-Left"), QStringLiteral("Two-Page Spread"),
            QStringLiteral("Fit to Window"), QStringLiteral("Zoom") }));
}

void TestToolBarApplicationMenu::emptyToolbarHidesReadingControls()
{
    ToolBarMenuFixture fixture = createOpenedCollectionScopeFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts, QStringList({ QStringLiteral("Fit to Window"), QStringLiteral("Zoom") }));
}

void TestToolBarApplicationMenu::directImageToolbarHidesReadingControls()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> imageDirectory = createImageDirectory(&sourcePath, &errorString);
    QVERIFY2(imageDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture
        = createOpenedCollectionScopeFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(imageDirectory);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts, QStringList({ QStringLiteral("Fit to Window"), QStringLiteral("Zoom") }));
}

void TestToolBarApplicationMenu::videoToolbarHidesReadingControlsAndDisablesImageControls()
{
    ToolBarMenuFixture fixture = createOpenedCollectionScopeFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY(fixture.root->setProperty("videoMode", true));
    QCoreApplication::processEvents();

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts, QStringList({ QStringLiteral("Fit to Window"), QStringLiteral("Zoom") }));

    bool invoked = false;
    const QVariantList enabledStates
        = invokeVariant(fixture.root, "toolbarControlEnabledStates", &invoked).toList();
    QVERIFY(invoked);
    QCOMPARE(enabledStates.size(), 2);
    for (const QVariant &enabledState : enabledStates) {
        QVERIFY(!enabledState.toBool());
    }
}

void TestToolBarApplicationMenu::comicArchiveToolbarShowsEnabledReadingControls()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> archiveDirectory
        = createComicBookArchive(&sourcePath, &errorString);
    QVERIFY2(archiveDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture
        = createOpenedCollectionScopeFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(archiveDirectory);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts,
        QStringList({ QStringLiteral("Right-to-Left"), QStringLiteral("Two-Page Spread"),
            QStringLiteral("Fit to Window"), QStringLiteral("Zoom") }));

    bool invoked = false;
    const QVariantList enabledStates
        = invokeVariant(fixture.root, "toolbarControlEnabledStates", &invoked).toList();
    QVERIFY(invoked);
    QCOMPARE(enabledStates.size(), 4);
    QVERIFY(enabledStates.at(0).toBool());
    QVERIFY(enabledStates.at(1).toBool());
}

void TestToolBarApplicationMenu::generalArchiveToolbarShowsDisabledReadingControls()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> archiveDirectory = createZipArchive(&sourcePath, &errorString);
    QVERIFY2(archiveDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture
        = createOpenedCollectionScopeFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(archiveDirectory);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts,
        QStringList({ QStringLiteral("Right-to-Left"), QStringLiteral("Two-Page Spread"),
            QStringLiteral("Fit to Window"), QStringLiteral("Zoom") }));

    bool invoked = false;
    const QVariantList enabledStates
        = invokeVariant(fixture.root, "toolbarControlEnabledStates", &invoked).toList();
    QVERIFY(invoked);
    QCOMPARE(enabledStates.size(), 4);
    QVERIFY(!enabledStates.at(0).toBool());
    QVERIFY(!enabledStates.at(1).toBool());
}

void TestToolBarApplicationMenu::directoryCollectionToolbarShowsDisabledReadingControls()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> imageDirectory
        = createDirectoryCollection(&sourcePath, &errorString);
    QVERIFY2(imageDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture
        = createOpenedCollectionScopeFixture(QUrl::fromLocalFile(sourcePath).toString());
    fixture.temporaryDirectory = std::move(imageDirectory);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    bool ok = false;
    const QStringList texts = invokeStringList(fixture.root, "toolbarControlTexts", &ok);
    QVERIFY(ok);
    QCOMPARE(texts,
        QStringList({ QStringLiteral("Right-to-Left"), QStringLiteral("Two-Page Spread"),
            QStringLiteral("Fit to Window"), QStringLiteral("Zoom") }));

    bool invoked = false;
    const QVariantList enabledStates
        = invokeVariant(fixture.root, "toolbarControlEnabledStates", &invoked).toList();
    QVERIFY(invoked);
    QCOMPARE(enabledStates.size(), 4);
    QVERIFY(!enabledStates.at(0).toBool());
    QVERIFY(!enabledStates.at(1).toBool());
}

void TestToolBarApplicationMenu::pageNavigationButtonsUseSemanticActionsForReadingDirection()
{
    QString sourcePath;
    QString errorString;
    std::unique_ptr<QTemporaryDir> imageDirectory
        = createDirectoryCollection(&sourcePath, &errorString);
    QVERIFY2(imageDirectory != nullptr, qPrintable(errorString));

    ToolBarMenuFixture fixture
        = createOpenedCollectionScopeFixture(QUrl::fromLocalFile(sourcePath).toString(), true);
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

void TestToolBarApplicationMenu::menubarGoMenuIconsFollowReadingDirection()
{
    ToolBarMenuFixture fixture = createMenuBarFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    const QStringList leftToRightIcons({
        QStringLiteral("go-previous"),
        QStringLiteral("go-next"),
        QStringLiteral("go-first-symbolic"),
        QStringLiteral("go-last-symbolic"),
        QStringLiteral("go-previous-use"),
        QStringLiteral("go-next-use"),
    });
    const QStringList rightToLeftIcons({
        QStringLiteral("go-previous"),
        QStringLiteral("go-next"),
        QStringLiteral("go-last-symbolic"),
        QStringLiteral("go-first-symbolic"),
        QStringLiteral("go-previous-use"),
        QStringLiteral("go-next-use"),
    });

    bool ok = false;
    QCOMPARE(invokeStringList(fixture.root, "goMenuActionIconNames", &ok), leftToRightIcons);
    QVERIFY(ok);

    fixture.root->setProperty("rightToLeftReadingActive", true);
    QCoreApplication::processEvents();

    QTRY_COMPARE(invokeStringList(fixture.root, "goMenuActionIconNames"), rightToLeftIcons);
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
