// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QString>
#include <QTest>
#include <QUrl>
#include <memory>

class TestMenuActionItem : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionTriggerUsesMenuItemClick();
    void controlTextDrivesMnemonicDisplay();
    void actionStateFollowsAction();
    void menuShortcutTextRendersWithoutRegisteringShortcut();
};

namespace {
struct MenuActionItemFixture {
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
import "%1" as KiriViewQml
import org.kde.kirigami as Kirigami

Item {
    id: root

    width: 400
    height: 320

    property int triggerCount: 0
    property int shortcutTriggerCount: 0
    property string inheritedIconName: inheritedStateItem.icon.name
    property string mnemonicLabel: overrideTextItem.Kirigami.MnemonicData.label

    function clickTriggerItem() {
        triggerItem.click();
    }

    Kirigami.Action {
        id: triggerAction

        text: "&Open"

        onTriggered: root.triggerCount += 1
    }

    KiriViewQml.MenuActionItem {
        id: triggerItem

        objectName: "triggerItem"

        action: triggerAction
        text: triggerAction.text
    }

    Kirigami.Action {
        id: textAction

        text: "&Open"
    }

    KiriViewQml.MenuActionItem {
        id: overrideTextItem

        objectName: "overrideTextItem"

        action: textAction
        text: "Save &As"
    }

    Kirigami.Action {
        id: stateAction

        checkable: true
        checked: true
        enabled: false
        icon.name: "document-open-symbolic"
        text: "&State"
    }

    KiriViewQml.MenuActionItem {
        id: inheritedStateItem

        objectName: "inheritedStateItem"

        action: stateAction
    }

    Kirigami.Action {
        id: shortcutDisplayAction

        property string menuShortcutText: "Ctrl+O"

        text: "&Shortcut"

        onTriggered: root.shortcutTriggerCount += 1
    }

    KiriViewQml.MenuActionItem {
        id: shortcutDisplayItem

        objectName: "shortcutDisplayItem"

        action: shortcutDisplayAction
        text: shortcutDisplayAction.text
    }
}
)")
        .arg(qmlSourceImport());
}

MenuActionItemFixture createFixture()
{
    MenuActionItemFixture fixture;
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(400, 320);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(
        fixtureQml().toUtf8(), QUrl(QStringLiteral("memory:test_menuactionitem.qml")));
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
        QUrl(QStringLiteral("memory:test_menuactionitem.qml")), &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    return fixture;
}

QObject *findObject(QObject *root, const QString &objectName)
{
    return root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
}
}

void TestMenuActionItem::actionTriggerUsesMenuItemClick()
{
    MenuActionItemFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QVERIFY(QMetaObject::invokeMethod(fixture.root, "clickTriggerItem", Qt::DirectConnection));

    QCOMPARE(fixture.root->property("triggerCount").toInt(), 1);
}

void TestMenuActionItem::controlTextDrivesMnemonicDisplay()
{
    MenuActionItemFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QCOMPARE(fixture.root->property("mnemonicLabel").toString(), QStringLiteral("Save &As"));

    QObject *item = findObject(fixture.root, QStringLiteral("overrideTextItem"));
    QVERIFY2(item != nullptr, "overrideTextItem was not created");
    QObject *label = findObject(item, QStringLiteral("menuActionItemTextLabel"));
    QVERIFY2(label != nullptr, "menuActionItemTextLabel was not created");
    const QString renderedText = label->property("text").toString();
    QVERIFY(renderedText.contains(QStringLiteral("Save")));
    QVERIFY(!renderedText.contains(QStringLiteral("Open")));
}

void TestMenuActionItem::actionStateFollowsAction()
{
    MenuActionItemFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QObject *inheritedItem = findObject(fixture.root, QStringLiteral("inheritedStateItem"));
    QVERIFY2(inheritedItem != nullptr, "inheritedStateItem was not created");
    QVERIFY(!inheritedItem->property("enabled").toBool());
    QVERIFY(inheritedItem->property("checkable").toBool());
    QVERIFY(inheritedItem->property("checked").toBool());
    QCOMPARE(fixture.root->property("inheritedIconName").toString(),
        QStringLiteral("document-open-symbolic"));
}

void TestMenuActionItem::menuShortcutTextRendersWithoutRegisteringShortcut()
{
    MenuActionItemFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QObject *item = findObject(fixture.root, QStringLiteral("shortcutDisplayItem"));
    QVERIFY2(item != nullptr, "shortcutDisplayItem was not created");
    QObject *shortcutLabel = findObject(item, QStringLiteral("menuActionItemShortcutLabel"));
    QVERIFY2(shortcutLabel != nullptr, "menuActionItemShortcutLabel was not created");

    QCOMPARE(item->property("menuShortcutText").toString(), QStringLiteral("Ctrl+O"));
    QCOMPARE(shortcutLabel->property("text").toString(), QStringLiteral("Ctrl+O"));
    QVERIFY(shortcutLabel->property("visible").toBool());

    QTest::keyClick(fixture.view.get(), Qt::Key_O, Qt::ControlModifier);
    QCoreApplication::processEvents();

    QCOMPARE(fixture.root->property("shortcutTriggerCount").toInt(), 0);
}

QTEST_MAIN(TestMenuActionItem)

#include "test_menuactionitem.moc"
