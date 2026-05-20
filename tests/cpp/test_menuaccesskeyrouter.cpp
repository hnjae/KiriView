// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/menuaccesskeyrouter.h"

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
    void altPressReleaseKeepsPopupOpen_data();
    void altPressReleaseKeepsPopupOpen();
    void altPressShowsMnemonicUnderline();
    void shortcutOverrideClaimsAltMnemonicWithoutTriggeringItem();
    void altMnemonicTriggersEnabledItem_data();
    void altMnemonicTriggersEnabledItem();
    void plainMnemonicTriggersEnabledItem_data();
    void plainMnemonicTriggersEnabledItem();
    void disabledMnemonicItemIgnored_data();
    void disabledMnemonicItemIgnored();
    void missingMnemonicItemIgnored_data();
    void missingMnemonicItemIgnored();
    void altMnemonicOpensNestedSubmenu();
    void nestedSubmenuMnemonicTriggersLeaf_data();
    void nestedSubmenuMnemonicTriggersLeaf();
    void menubarAltMnemonicOpensNestedSubmenu();
    void menubarNestedSubmenuMnemonicTriggersLeaf_data();
    void menubarNestedSubmenuMnemonicTriggersLeaf();
    void menubarHeldAltMnemonicOpensNestedSubmenu();
};

namespace {
struct MenuFixture {
    std::unique_ptr<QQuickView> view;
    QObject *root = nullptr;
    QObject *menu = nullptr;
    QObject *subMenu = nullptr;
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr && menu != nullptr; }
};

void addPopupFixtureRows()
{
    QTest::addColumn<bool>("actionHasShortcut");
    QTest::addColumn<bool>("customMenuItem");
    QTest::newRow("popup-menu") << false << false;
    QTest::newRow("popup-menu-action-shortcut") << true << false;
    QTest::newRow("popup-menu-custom-item") << false << true;
    QTest::newRow("popup-menu-custom-item-action-shortcut") << true << true;
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

QString boolLiteral(bool value) { return value ? QStringLiteral("true") : QStringLiteral("false"); }

QString popupMenuFixtureQml(
    bool actionHasShortcut, bool customMenuItem, bool actionEnabled, const QString &actionText)
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

        enabled: %2
        shortcut: "%3"
        text: "%4"

        onTriggered: root.triggerCount += 1
    }

    Controls.Button {
        id: menuButton

        text: "Application Menu"
    }

    Controls.Menu {
        id: targetMenu

        objectName: "targetMenu"

        %5 {
            action: openAction
            text: openAction.text
        }
    }
}
)")
        .arg(qmlSourceImport(), boolLiteral(actionEnabled),
            actionHasShortcut ? QStringLiteral("Ctrl+O") : QString(), actionText,
            menuItemType(customMenuItem));
}

QString nestedSubmenuFixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls

Item {
    id: root

    width: 320
    height: 240

    property int fitHeightTriggerCount: 0

    function openTargetMenu() {
        targetMenu.popup(menuButton, 0, menuButton.height);
    }

    Controls.Button {
        id: menuButton

        text: "Application Menu"
    }

    Controls.Menu {
        id: targetMenu

        objectName: "targetMenu"

        Controls.MenuItem {
            text: "&Open"
        }

        Controls.Menu {
            id: fitMenu

            objectName: "fitMenu"
            title: "F&it"

            Controls.MenuItem {
                text: "Fit &Height"

                onTriggered: root.fitHeightTriggerCount += 1
            }
        }
    }
}
)");
}

QString menubarFixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls

Item {
    id: root

    width: 360
    height: 240

    property int fitHeightTriggerCount: 0

    Controls.MenuBar {
        id: menuBar

        focus: true
        objectName: "menuBar"

        Component.onCompleted: forceActiveFocus()

        Controls.Menu {
            title: "&File"

            Controls.MenuItem {
                text: "&Open"
            }
        }

        Controls.Menu {
            id: viewMenu

            objectName: "targetMenu"
            title: "&View"

            Controls.Menu {
                id: fitMenu

                objectName: "fitMenu"
                title: "F&it"

                Controls.MenuItem {
                    text: "Fit &Height"

                    onTriggered: root.fitHeightTriggerCount += 1
                }
            }
        }
    }
}
)");
}

MenuFixture createPopupMenuFixture(bool actionHasShortcut, bool customMenuItem,
    bool actionEnabled = true, const QString &actionText = QStringLiteral("&Open"))
{
    MenuFixture fixture;
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(320, 240);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(
        popupMenuFixtureQml(actionHasShortcut, customMenuItem, actionEnabled, actionText).toUtf8(),
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

    return fixture;
}

MenuFixture createMenuFixture(const QString &source)
{
    MenuFixture fixture;
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(360, 240);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(source.toUtf8(), QUrl(QStringLiteral("memory:test_menuaccesskeyrouter.qml")));
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
    fixture.subMenu
        = root->findChild<QObject *>(QStringLiteral("fitMenu"), Qt::FindChildrenRecursively);
    if (fixture.menu == nullptr) {
        fixture.errorString = QStringLiteral("target menu was not created");
    } else if (fixture.subMenu == nullptr) {
        fixture.errorString = QStringLiteral("fit submenu was not created");
    }

    return fixture;
}

MenuFixture createNestedSubmenuFixture() { return createMenuFixture(nestedSubmenuFixtureQml()); }

MenuFixture createMenubarFixture() { return createMenuFixture(menubarFixtureQml()); }

bool isMenuOpen(QObject *menu)
{
    return menu != nullptr
        && (menu->property("opened").toBool() || menu->property("visible").toBool());
}

bool sendKey(QObject *target, QEvent::Type type, Qt::Key key, Qt::KeyboardModifiers modifiers,
    const QString &text = {})
{
    QKeyEvent event(type, key, modifiers, text);
    QCoreApplication::sendEvent(target, &event);
    QCoreApplication::processEvents();
    return event.isAccepted();
}

void keyClick(QQuickView *view, Qt::Key key,
    Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(Qt::NoModifier))
{
    QTest::keyClick(view, key, modifiers);
    QCoreApplication::processEvents();
}

void keyPress(QQuickView *view, Qt::Key key,
    Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(Qt::NoModifier))
{
    QTest::keyPress(view, key, modifiers);
    QCoreApplication::processEvents();
}

void keyRelease(QQuickView *view, Qt::Key key,
    Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(Qt::NoModifier))
{
    QTest::keyRelease(view, key, modifiers);
    QCoreApplication::processEvents();
}

void openTargetMenu(QObject *root)
{
    QVERIFY(QMetaObject::invokeMethod(root, "openTargetMenu", Qt::DirectConnection));
}

QObject *findObject(QObject *root, const QString &objectName)
{
    return root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
}
}

void TestMenuAccessKeyRouter::altPressReleaseKeepsPopupOpen_data() { addPopupFixtureRows(); }

void TestMenuAccessKeyRouter::altPressReleaseKeepsPopupOpen()
{
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createPopupMenuFixture(actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    keyPress(fixture.view.get(), Qt::Key_Alt, Qt::AltModifier);
    QVERIFY(isMenuOpen(fixture.menu));
    keyRelease(fixture.view.get(), Qt::Key_Alt);

    QVERIFY(isMenuOpen(fixture.menu));
}

void TestMenuAccessKeyRouter::altPressShowsMnemonicUnderline()
{
    MenuFixture fixture = createPopupMenuFixture(false, true);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    QObject *label = findObject(fixture.root, QStringLiteral("menuActionItemTextLabel"));
    QVERIFY2(label != nullptr, "menu action item text label was not created");
    const QString initialText = label->property("text").toString();

    keyPress(fixture.view.get(), Qt::Key_Alt, Qt::AltModifier);
    QTRY_VERIFY(label->property("text").toString() != initialText);
    QVERIFY(label->property("text").toString().contains(QStringLiteral("<u>")));

    keyRelease(fixture.view.get(), Qt::Key_Alt);
    QVERIFY(isMenuOpen(fixture.menu));
    QTRY_COMPARE(label->property("text").toString(), initialText);
}

void TestMenuAccessKeyRouter::shortcutOverrideClaimsAltMnemonicWithoutTriggeringItem()
{
    MenuFixture fixture = createPopupMenuFixture(false, true);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    QObject *label = findObject(fixture.root, QStringLiteral("menuActionItemTextLabel"));
    QVERIFY2(label != nullptr, "menu action item text label was not created");

    QVERIFY(sendKey(fixture.menu, QEvent::ShortcutOverride, Qt::Key_O, Qt::AltModifier));

    QTRY_VERIFY(label->property("text").toString().contains(QStringLiteral("<u>")));
    QCOMPARE(fixture.root->property("triggerCount").toInt(), 0);

    keyRelease(fixture.view.get(), Qt::Key_Alt);
    QTRY_VERIFY(!label->property("text").toString().contains(QStringLiteral("<u>")));
}

void TestMenuAccessKeyRouter::altMnemonicTriggersEnabledItem_data() { addPopupFixtureRows(); }

void TestMenuAccessKeyRouter::altMnemonicTriggersEnabledItem()
{
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createPopupMenuFixture(actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    keyClick(fixture.view.get(), Qt::Key_O, Qt::AltModifier);

    QTRY_COMPARE(fixture.root->property("triggerCount").toInt(), 1);
    QTest::qWait(50);
    QCOMPARE(fixture.root->property("triggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::plainMnemonicTriggersEnabledItem_data() { addPopupFixtureRows(); }

void TestMenuAccessKeyRouter::plainMnemonicTriggersEnabledItem()
{
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture = createPopupMenuFixture(actionHasShortcut, customMenuItem);
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    keyClick(fixture.view.get(), Qt::Key_O);

    QTRY_COMPARE(fixture.root->property("triggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::disabledMnemonicItemIgnored_data() { addPopupFixtureRows(); }

void TestMenuAccessKeyRouter::disabledMnemonicItemIgnored()
{
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture
        = createPopupMenuFixture(actionHasShortcut, customMenuItem, false, QStringLiteral("&Open"));
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_O, Qt::AltModifier);

    QCOMPARE(fixture.root->property("triggerCount").toInt(), 0);
    QVERIFY(isMenuOpen(fixture.menu));
}

void TestMenuAccessKeyRouter::altMnemonicOpensNestedSubmenu()
{
    MenuFixture fixture = createNestedSubmenuFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_I, Qt::AltModifier);

    QTRY_VERIFY(isMenuOpen(fixture.menu));
    QTRY_VERIFY(isMenuOpen(fixture.subMenu));
}

void TestMenuAccessKeyRouter::nestedSubmenuMnemonicTriggersLeaf_data()
{
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");

    QTest::newRow("alt-mnemonic") << Qt::KeyboardModifiers(Qt::AltModifier);
    QTest::newRow("plain-mnemonic") << Qt::KeyboardModifiers(Qt::NoModifier);
}

void TestMenuAccessKeyRouter::nestedSubmenuMnemonicTriggersLeaf()
{
    QFETCH(Qt::KeyboardModifiers, modifiers);
    MenuFixture fixture = createNestedSubmenuFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_I, Qt::AltModifier);
    QTRY_VERIFY(isMenuOpen(fixture.subMenu));

    sendKey(fixture.subMenu, QEvent::KeyPress, Qt::Key_H, modifiers);

    QTRY_COMPARE(fixture.root->property("fitHeightTriggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::menubarAltMnemonicOpensNestedSubmenu()
{
    MenuFixture fixture = createMenubarFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    keyClick(fixture.view.get(), Qt::Key_V, Qt::AltModifier);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    keyClick(fixture.view.get(), Qt::Key_I, Qt::AltModifier);

    QTRY_VERIFY(isMenuOpen(fixture.menu));
    QTRY_VERIFY(isMenuOpen(fixture.subMenu));
}

void TestMenuAccessKeyRouter::menubarNestedSubmenuMnemonicTriggersLeaf_data()
{
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");

    QTest::newRow("alt-mnemonic") << Qt::KeyboardModifiers(Qt::AltModifier);
    QTest::newRow("plain-mnemonic") << Qt::KeyboardModifiers(Qt::NoModifier);
}

void TestMenuAccessKeyRouter::menubarNestedSubmenuMnemonicTriggersLeaf()
{
    QFETCH(Qt::KeyboardModifiers, modifiers);
    MenuFixture fixture = createMenubarFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    keyClick(fixture.view.get(), Qt::Key_V, Qt::AltModifier);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    keyClick(fixture.view.get(), Qt::Key_I, Qt::AltModifier);
    QTRY_VERIFY(isMenuOpen(fixture.subMenu));

    keyClick(fixture.view.get(), Qt::Key_H, modifiers);

    QTRY_COMPARE(fixture.root->property("fitHeightTriggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::menubarHeldAltMnemonicOpensNestedSubmenu()
{
    MenuFixture fixture = createMenubarFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    keyPress(fixture.view.get(), Qt::Key_Alt, Qt::AltModifier);
    keyClick(fixture.view.get(), Qt::Key_V, Qt::AltModifier);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    keyClick(fixture.view.get(), Qt::Key_I, Qt::AltModifier);
    QTRY_VERIFY(isMenuOpen(fixture.menu));
    QTRY_VERIFY(isMenuOpen(fixture.subMenu));

    keyRelease(fixture.view.get(), Qt::Key_Alt);

    QVERIFY(isMenuOpen(fixture.menu));
    QVERIFY(isMenuOpen(fixture.subMenu));
}

void TestMenuAccessKeyRouter::missingMnemonicItemIgnored_data() { addPopupFixtureRows(); }

void TestMenuAccessKeyRouter::missingMnemonicItemIgnored()
{
    QFETCH(bool, actionHasShortcut);
    QFETCH(bool, customMenuItem);
    MenuFixture fixture
        = createPopupMenuFixture(actionHasShortcut, customMenuItem, true, QStringLiteral("&Open"));
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setMenu(fixture.menu);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    keyClick(fixture.view.get(), Qt::Key_X, Qt::AltModifier);

    QCOMPARE(fixture.root->property("triggerCount").toInt(), 0);
    QVERIFY(isMenuOpen(fixture.menu));
}

QTEST_MAIN(TestMenuAccessKeyRouter)

#include "test_menuaccesskeyrouter.moc"
