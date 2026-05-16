// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeyrouter.h"

#include <QCoreApplication>
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
    void altPressDoesNotImmediatelyCloseMenu_data();
    void altPressDoesNotImmediatelyCloseMenu();
    void altMnemonicTriggersItem_data();
    void altMnemonicTriggersItem();
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
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr && menu != nullptr; }
};

void addFixtureRows()
{
    QTest::addColumn<int>("fixtureKind");
    QTest::newRow("menubar") << static_cast<int>(FixtureKind::MenuBar);
    QTest::newRow("popup-menu") << static_cast<int>(FixtureKind::PopupMenu);
}

void addEnvironmentImportPaths(QQmlEngine &engine)
{
    const QString paths = qEnvironmentVariable("NIXPKGS_QML_SEARCH_PATHS");
    for (const QString &path : paths.split(QLatin1Char(':'), Qt::SkipEmptyParts)) {
        engine.addImportPath(path);
    }
}

QString menuBarFixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls

Item {
    id: root

    width: 320
    height: 240

    property int triggerCount: 0

    function openTargetMenu() {
        targetMenu.popup(menuBar);
    }

    Controls.Action {
        id: openAction

        text: "&Open"

        onTriggered: root.triggerCount += 1
    }

    Controls.MenuBar {
        id: menuBar

        Controls.Menu {
            id: targetMenu

            objectName: "targetMenu"
            title: "&File"

            Controls.MenuItem {
                action: openAction
                text: openAction.text
            }
        }
    }
}
)");
}

QString popupMenuFixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls

Item {
    id: root

    width: 320
    height: 240

    property int triggerCount: 0

    function openTargetMenu() {
        targetMenu.popup(menuButton);
    }

    Controls.Action {
        id: openAction

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

        Controls.MenuItem {
            action: openAction
            text: openAction.text
        }
    }
}
)");
}

QString fixtureQml(FixtureKind kind)
{
    switch (kind) {
    case FixtureKind::MenuBar:
        return menuBarFixtureQml();
    case FixtureKind::PopupMenu:
        return popupMenuFixtureQml();
    }

    return {};
}

MenuFixture createMenuFixture(FixtureKind kind)
{
    MenuFixture fixture;
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(320, 240);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(
        fixtureQml(kind).toUtf8(), QUrl(QStringLiteral("memory:test_menuaccesskeyrouter.qml")));
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

void TestMenuAccessKeyRouter::altPressDoesNotImmediatelyCloseMenu_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::altPressDoesNotImmediatelyCloseMenu()
{
    QFETCH(int, fixtureKind);
    MenuFixture fixture = createMenuFixture(static_cast<FixtureKind>(fixtureKind));
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
    MenuFixture fixture = createMenuFixture(static_cast<FixtureKind>(fixtureKind));
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    MenuAccessKeyRouter router;
    router.setRootObject(fixture.root);

    openTargetMenu(fixture.root);
    QTRY_VERIFY(isMenuOpen(fixture.menu));

    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_Alt, Qt::AltModifier);
    sendKey(fixture.menu, QEvent::KeyPress, Qt::Key_O, Qt::AltModifier, QStringLiteral("o"));

    QTRY_COMPARE(fixture.root->property("triggerCount").toInt(), 1);
}

void TestMenuAccessKeyRouter::plainMnemonicStillTriggersItem_data() { addFixtureRows(); }

void TestMenuAccessKeyRouter::plainMnemonicStillTriggersItem()
{
    QFETCH(int, fixtureKind);
    MenuFixture fixture = createMenuFixture(static_cast<FixtureKind>(fixtureKind));
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
    MenuFixture fixture = createMenuFixture(static_cast<FixtureKind>(fixtureKind));
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
