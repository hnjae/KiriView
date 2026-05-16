// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeyrouter.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QKeyEvent>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QString>
#include <QTest>
#include <QUrl>
#include <memory>

class TestMenuAccessKeyRouter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void altMnemonicOpensMenuBarMenu_data();
    void altMnemonicOpensMenuBarMenu();
    void altPressDoesNotImmediatelyCloseMenu_data();
    void altPressDoesNotImmediatelyCloseMenu();
    void altMnemonicTriggersItem_data();
    void altMnemonicTriggersItem();
    void altModifiedKeyClickTriggersItem_data();
    void altModifiedKeyClickTriggersItem();
    void altMnemonicOpensSubMenu_data();
    void altMnemonicOpensSubMenu();
    void plainMnemonicStillTriggersItem_data();
    void plainMnemonicStillTriggersItem();
    void altPressReleaseKeepsMenuOpen_data();
    void altPressReleaseKeepsMenuOpen();
};

namespace {
enum class FixtureKind {
    MenuBar,
    PopupMenu,
};

struct MenuFixture {
    std::unique_ptr<QQuickView> view;
    QObject *root = nullptr;
    QObject *menu = nullptr;
    QObject *subMenu = nullptr;
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr && menu != nullptr; }
};

void addFixtureRows()
{
    QTest::addColumn<int>("fixtureKind");
    QTest::addColumn<bool>("actionHasShortcut");
    QTest::addColumn<bool>("customMenuItem");
    QTest::newRow("menubar") << static_cast<int>(FixtureKind::MenuBar) << false << false;
    QTest::newRow("menubar-action-shortcut")
        << static_cast<int>(FixtureKind::MenuBar) << true << false;
    QTest::newRow("menubar-custom-item") << static_cast<int>(FixtureKind::MenuBar) << false << true;
    QTest::newRow("menubar-custom-item-action-shortcut")
        << static_cast<int>(FixtureKind::MenuBar) << true << true;
    QTest::newRow("popup-menu") << static_cast<int>(FixtureKind::PopupMenu) << false << false;
    QTest::newRow("popup-menu-action-shortcut")
        << static_cast<int>(FixtureKind::PopupMenu) << true << false;
    QTest::newRow("popup-menu-custom-item")
        << static_cast<int>(FixtureKind::PopupMenu) << false << true;
    QTest::newRow("popup-menu-custom-item-action-shortcut")
        << static_cast<int>(FixtureKind::PopupMenu) << true << true;
}

void addMenuBarFixtureRows()
{
    QTest::addColumn<bool>("actionHasShortcut");
    QTest::addColumn<bool>("customMenuItem");
    QTest::newRow("menubar") << false << false;
    QTest::newRow("menubar-action-shortcut") << true << false;
    QTest::newRow("menubar-custom-item") << false << true;
    QTest::newRow("menubar-custom-item-action-shortcut") << true << true;
}

void addEnvironmentImportPaths(QQmlEngine &engine)
{
    const QString paths = qEnvironmentVariable("NIXPKGS_QML_SEARCH_PATHS");
    for (const QString &path : paths.split(QLatin1Char(':'), Qt::SkipEmptyParts)) {
        engine.addImportPath(path);
    }
}

QString qmlSourceImport()
{
    const QString qmlPath = QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
                                .absoluteFilePath(QStringLiteral("../../src/qml"));
    return QUrl::fromLocalFile(qmlPath).toString();
}

QString menuItemType(bool customMenuItem)
{
    return customMenuItem ? QStringLiteral("KiriViewQml.MenuActionItem")
                          : QStringLiteral("Controls.MenuItem");
}

QString menuBarFixtureQml(bool actionHasShortcut, bool customMenuItem)
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls
import "%1" as KiriViewQml
import org.kde.kirigami as Kirigami

Item {
    id: root

    width: 320
    height: 240

    property int triggerCount: 0

    function openTargetMenu() {
        targetMenu.popup(menuBar);
    }

    Kirigami.Action {
        id: openAction

        shortcut: "%2"
        text: "&Open"

        onTriggered: root.triggerCount += 1
    }

    Controls.MenuBar {
        id: menuBar

        Controls.Menu {
            id: targetMenu

            objectName: "targetMenu"
            title: "&File"

            %3 {
                action: openAction
                text: openAction.text
            }
        }
    }
}
)")
        .arg(qmlSourceImport(), actionHasShortcut ? QStringLiteral("Ctrl+O") : QString(),
            menuItemType(customMenuItem));
}

QString popupMenuFixtureQml(bool actionHasShortcut, bool customMenuItem)
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls
import "%1" as KiriViewQml
import org.kde.kirigami as Kirigami

Item {
    id: root

    width: 320
    height: 240

    property int triggerCount: 0

    function openTargetMenu() {
        targetMenu.popup(menuButton);
    }

    Kirigami.Action {
        id: openAction

        shortcut: "%2"
        text: "&Open"

        onTriggered: root.triggerCount += 1
    }

    Controls.Button {
        id: menuButton

        text: "Application Menu"
    }

    Controls.Menu {
        id: targetMenu

        objectName: "targetMenu"

        %3 {
            action: openAction
            text: openAction.text
        }
    }
}
)")
        .arg(qmlSourceImport(), actionHasShortcut ? QStringLiteral("Ctrl+O") : QString(),
            menuItemType(customMenuItem));
}

QString subMenuFixtureQml(FixtureKind kind, bool actionHasShortcut, bool customMenuItem)
{
    const QString anchorItem
        = kind == FixtureKind::MenuBar ? QStringLiteral("menuBar") : QStringLiteral("menuButton");
    const QString button = kind == FixtureKind::MenuBar ? QString() : QStringLiteral(R"(
    Controls.Button {
        id: menuButton

        text: "Application Menu"
    }
)");
    const QString menuContainerStart = kind == FixtureKind::MenuBar ? QStringLiteral(R"(
    Controls.MenuBar {
        id: menuBar

)")
                                                                    : QString();
    const QString menuContainerEnd = kind == FixtureKind::MenuBar ? QStringLiteral(R"(
    }
)")
                                                                  : QString();

    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls
import "%1" as KiriViewQml
import org.kde.kirigami as Kirigami

Item {
    id: root

    width: 320
    height: 240

    property int triggerCount: 0

    function openTargetMenu() {
        targetMenu.popup(%2);
    }

    Kirigami.Action {
        id: openAction

        shortcut: "%3"
        text: "&Open"

        onTriggered: root.triggerCount += 1
    }

%4
%5
    Controls.Menu {
        id: targetMenu

        objectName: "targetMenu"
        title: "&View"

        %6 {
            action: openAction
            text: openAction.text
        }

        Controls.Menu {
            id: subMenu

            objectName: "subMenu"
            title: "&Fit"

            Controls.MenuItem {
                text: "&Width"
            }
        }
    }
%7
}
)")
        .arg(qmlSourceImport(), anchorItem,
            actionHasShortcut ? QStringLiteral("Ctrl+O") : QString(), button, menuContainerStart,
            menuItemType(customMenuItem), menuContainerEnd);
}

QString fixtureQml(FixtureKind kind, bool actionHasShortcut, bool customMenuItem)
{
    switch (kind) {
    case FixtureKind::MenuBar:
        return menuBarFixtureQml(actionHasShortcut, customMenuItem);
    case FixtureKind::PopupMenu:
        return popupMenuFixtureQml(actionHasShortcut, customMenuItem);
    }

    return {};
}

MenuFixture createMenuFixture(
    FixtureKind kind, bool actionHasShortcut, bool customMenuItem, bool includeSubMenu = false)
{
    MenuFixture fixture;
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(320, 240);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData((includeSubMenu ? subMenuFixtureQml(kind, actionHasShortcut, customMenuItem)
                                      : fixtureQml(kind, actionHasShortcut, customMenuItem))
                          .toUtf8(),
        QUrl(QStringLiteral("memory:test_menuaccesskeyrouter.qml")));
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
        QUrl(QStringLiteral("memory:test_menuaccesskeyrouter.qml")), &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    fixture.menu
        = root->findChild<QObject *>(QStringLiteral("targetMenu"), Qt::FindChildrenRecursively);
    if (fixture.menu == nullptr) {
        fixture.errorString = QStringLiteral("target menu was not created");
    }
    fixture.subMenu
        = root->findChild<QObject *>(QStringLiteral("subMenu"), Qt::FindChildrenRecursively);

    return fixture;
}

bool isMenuOpen(QObject *menu)
{
    return menu != nullptr
        && (menu->property("opened").toBool() || menu->property("visible").toBool());
}

void sendKey(QObject *target, QEvent::Type type, Qt::Key key, Qt::KeyboardModifiers modifiers,
    const QString &text = {})
{
    QKeyEvent event(type, key, modifiers, text);
    QCoreApplication::sendEvent(target, &event);
    QCoreApplication::processEvents();
}

void openTargetMenu(QObject *root)
{
    QVERIFY(QMetaObject::invokeMethod(root, "openTargetMenu", Qt::DirectConnection));
}
}

void TestMenuAccessKeyRouter::altMnemonicOpensMenuBarMenu_data() { addMenuBarFixtureRows(); }

void TestMenuAccessKeyRouter::altMnemonicOpensMenuBarMenu()
{
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture
        = createMenuFixture(FixtureKind::MenuBar, actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    QTest::keyClick(fixture.view.get(), Qt::Key_F, Qt::AltModifier);

    QTRY_VERIFY(isMenuOpen(fixture.menu));
}

void TestMenuAccessKeyRouter::altPressDoesNotImmediatelyCloseMenu_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::altPressDoesNotImmediatelyCloseMenu()
{
    QFETCH(int, fixtureKind);
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createMenuFixture(
        static_cast<FixtureKind>(fixtureKind), actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_Alt, Qt::AltModifier);

    QVERIFY(isMenuOpen(fixture.menu));
}

void TestMenuAccessKeyRouter::altMnemonicTriggersItem_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::altMnemonicTriggersItem()
{
    QFETCH(int, fixtureKind);
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createMenuFixture(
        static_cast<FixtureKind>(fixtureKind), actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_Alt, Qt::AltModifier);
    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_O, Qt::AltModifier, QStringLiteral("o"));

    QTRY_COMPARE(fixture.root->property("triggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::altModifiedKeyClickTriggersItem_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::altModifiedKeyClickTriggersItem()
{
    QFETCH(int, fixtureKind);
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createMenuFixture(
        static_cast<FixtureKind>(fixtureKind), actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    QTest::keyClick(fixture.view.get(), Qt::Key_O, Qt::AltModifier);
    QCoreApplication::processEvents();

    QTRY_COMPARE(fixture.root->property("triggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::altMnemonicOpensSubMenu_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::altMnemonicOpensSubMenu()
{
    QFETCH(int, fixtureKind);
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createMenuFixture(
        static_cast<FixtureKind>(fixtureKind), actionHasShortcut, customMenuItem, true);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QVERIFY(fixture.subMenu != nullptr);

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_Alt, Qt::AltModifier);
    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_F, Qt::AltModifier, QStringLiteral("f"));

    QTRY_VERIFY(isMenuOpen(fixture.subMenu));
}

void TestMenuAccessKeyRouter::plainMnemonicStillTriggersItem_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::plainMnemonicStillTriggersItem()
{
    QFETCH(int, fixtureKind);
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createMenuFixture(
        static_cast<FixtureKind>(fixtureKind), actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    QTest::keyClick(fixture.view.get(), Qt::Key_O, Qt::NoModifier);
    QCoreApplication::processEvents();

    QTRY_COMPARE(fixture.root->property("triggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::altPressReleaseKeepsMenuOpen_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::altPressReleaseKeepsMenuOpen()
{
    QFETCH(int, fixtureKind);
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createMenuFixture(
        static_cast<FixtureKind>(fixtureKind), actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_Alt, Qt::AltModifier);
    QVERIFY(isMenuOpen(fixture.menu));
    sendKey(fixture.menu, QEvent::KeyRelease, Qt::Key_Alt, Qt::NoModifier);

    QVERIFY(isMenuOpen(fixture.menu));
}

QTEST_MAIN(TestMenuAccessKeyRouter)

#include "test_menuaccesskeyrouter.moc"
