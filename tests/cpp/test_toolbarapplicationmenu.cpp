// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedocument.h"
#include "localization.h"
#include "menuaccesskeyrouter.h"

#include <KLocalizedQmlContext>
#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QString>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>
#include <memory>

class TestToolBarApplicationMenu : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buttonClickTogglesMenu();
    void outsideClickClosesMenu();
    void openApplicationMenuOnlyOpens();
    void mnemonicRoutingStillTriggersMenuAction();
};

namespace {
struct ToolBarMenuFixture {
    std::unique_ptr<QQuickView> view;
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

QString fixtureQml()
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

    function applicationMenuButton() {
        return toolbar.findActionButton(toolbar, toolbar.applicationMenuAction);
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

    KiriImageDocument {
        id: imageDocument
    }

    Kirigami.Action {
        id: previousImageKirigamiAction

        enabled: false
        icon.name: "go-previous-symbolic"
        text: "Previous Image"
    }

    Kirigami.Action {
        id: nextImageKirigamiAction

        enabled: false
        icon.name: "go-next-symbolic"
        text: "Next Image"
    }

    Kirigami.Action {
        id: twoPageModeKirigamiAction

        enabled: false
        icon.name: "view-split-left-right-symbolic"
        text: "Two-Page Spread"
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
        imageReady: false
        maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
        minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
        showApplicationMenuActions: true
        zoomStepFactor: imageDocument.zoomStepFactor
    }
}
)")
        .arg(qmlSourceImport());
}

ToolBarMenuFixture createFixture()
{
    ToolBarMenuFixture fixture;
    registerKiriViewQmlTypes();
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(720, 420);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());
    KLocalization::setupLocalizedContext(fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(
        fixtureQml().toUtf8(), QUrl(QStringLiteral("memory:test_toolbarapplicationmenu.qml")));
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
        QUrl(QStringLiteral("memory:test_toolbarapplicationmenu.qml")), &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    return fixture;
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

void clickAt(QQuickView *view, const QPoint &point)
{
    QVERIFY(point.x() >= 0);
    QVERIFY(point.y() >= 0);

    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, point);
    QCoreApplication::processEvents();
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

void TestToolBarApplicationMenu::buttonClickTogglesMenu()
{
    ToolBarMenuFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "hasApplicationMenuButton"));

    clickApplicationMenuButton(fixture);
    QTRY_VERIFY(applicationMenuOpen(fixture.root));

    clickApplicationMenuButton(fixture);
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

QTEST_MAIN(TestToolBarApplicationMenu)

#include "test_toolbarapplicationmenu.moc"
