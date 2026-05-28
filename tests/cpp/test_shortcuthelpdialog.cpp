// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriviewapplication.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
#include <KSharedConfig>
#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QPoint>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>
#include <memory>

class TestShortcutHelpDialog : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void escapeClosesDialog();
    void enterAcceptsAndClosesDialog();
    void outsideClickClosesDialog();
    void presentsConfigurableActionShortcutsAsFormCards();
};

namespace {
struct ShortcutHelpDialogFixture {
    std::unique_ptr<QQuickView> view;
    QObject *root = nullptr;
    QObject *dialog = nullptr;
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr && dialog != nullptr; }
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

    qmlRegisterType<KiriViewApplication>("io.github.hnjae.kiriview", 1, 0, "KiriViewApplication");
    registered = true;
}

void resetConfig()
{
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

QString fixtureQml()
{
    return QStringLiteral(R"(
import QtQuick
import io.github.hnjae.kiriview
import "%1" as KiriViewQml

Item {
    id: root

    focus: true
    width: 900
    height: 700

    property int acceptedCount: 0
    property int closedCount: 0
    property bool dialogOpened: shortcutHelpDialog.opened

    function openDialog() {
        shortcutHelpDialog.open();
    }

    Component.onCompleted: forceActiveFocus()

    KiriViewApplication {
        id: application
    }

    KiriViewQml.ShortcutHelpDialog {
        id: shortcutHelpDialog

        objectName: "shortcutHelpDialog"
        parent: root
        application: application

        onAccepted: root.acceptedCount += 1
        onClosed: root.closedCount += 1
    }
}
)")
        .arg(qmlSourceImport());
}

ShortcutHelpDialogFixture createFixture()
{
    ShortcutHelpDialogFixture fixture;
    registerKiriViewQmlTypes();
    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(900, 700);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());
    KLocalization::setupLocalizedContext(fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(
        fixtureQml().toUtf8(), QUrl(QStringLiteral("memory:test_shortcuthelpdialog.qml")));
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
        QUrl(QStringLiteral("memory:test_shortcuthelpdialog.qml")), &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    fixture.dialog = root->findChild<QObject *>(QStringLiteral("shortcutHelpDialog"));
    if (fixture.dialog == nullptr) {
        fixture.errorString = QStringLiteral("shortcut help dialog was not created");
    }
    return fixture;
}

bool invokeRoot(QObject *root, const char *method)
{
    return QMetaObject::invokeMethod(root, method, Qt::DirectConnection);
}

bool dialogOpened(QObject *root) { return root->property("dialogOpened").toBool(); }

int intProperty(QObject *root, const char *name) { return root->property(name).toInt(); }

void openDialog(ShortcutHelpDialogFixture &fixture)
{
    QVERIFY(invokeRoot(fixture.root, "openDialog"));
    QTRY_VERIFY(dialogOpened(fixture.root));
}

void collectChildProperties(
    QQuickItem *root, const QString &objectName, const char *propertyName, QStringList &values)
{
    if (root == nullptr) {
        return;
    }

    if (root->objectName() == objectName) {
        values.push_back(root->property(propertyName).toString());
    }

    for (QQuickItem *child : root->childItems()) {
        collectChildProperties(child, objectName, propertyName, values);
    }
}

QStringList childTexts(QQuickItem *root, const QString &objectName)
{
    QStringList texts;
    collectChildProperties(root, objectName, "text", texts);
    return texts;
}

QStringList childTitles(QQuickItem *root, const QString &objectName)
{
    QStringList titles;
    collectChildProperties(root, objectName, "title", titles);
    return titles;
}

int childObjectCount(QQuickItem *root, const QString &objectName)
{
    if (root == nullptr) {
        return 0;
    }

    int count = root->objectName() == objectName ? 1 : 0;
    for (QQuickItem *child : root->childItems()) {
        count += childObjectCount(child, objectName);
    }
    return count;
}
}

void TestShortcutHelpDialog::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KiriView::initializeLocalization();
    resetConfig();
}

void TestShortcutHelpDialog::init() { resetConfig(); }

void TestShortcutHelpDialog::cleanup() { resetConfig(); }

void TestShortcutHelpDialog::escapeClosesDialog()
{
    ShortcutHelpDialogFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    openDialog(fixture);

    QTest::keyClick(fixture.view.get(), Qt::Key_Escape);
    QCoreApplication::processEvents();

    QTRY_VERIFY(!dialogOpened(fixture.root));
    QTRY_COMPARE(intProperty(fixture.root, "closedCount"), 1);
}

void TestShortcutHelpDialog::enterAcceptsAndClosesDialog()
{
    ShortcutHelpDialogFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    openDialog(fixture);

    QTest::keyClick(fixture.view.get(), Qt::Key_Return);
    QCoreApplication::processEvents();

    QTRY_VERIFY(!dialogOpened(fixture.root));
    QCOMPARE(intProperty(fixture.root, "acceptedCount"), 1);
    QTRY_COMPARE(intProperty(fixture.root, "closedCount"), 1);
}

void TestShortcutHelpDialog::outsideClickClosesDialog()
{
    ShortcutHelpDialogFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    openDialog(fixture);

    QTest::mouseClick(fixture.view.get(), Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QCoreApplication::processEvents();

    QTRY_VERIFY(!dialogOpened(fixture.root));
    QTRY_COMPARE(intProperty(fixture.root, "closedCount"), 1);
}

void TestShortcutHelpDialog::presentsConfigurableActionShortcutsAsFormCards()
{
    ShortcutHelpDialogFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    openDialog(fixture);

    QVERIFY(fixture.view->contentItem()->findChild<QObject *>(QStringLiteral("shortcutHelpForm"))
        == nullptr);
    QVERIFY(
        fixture.view->contentItem()->findChild<QObject *>(QStringLiteral("shortcutHelpScrollView"))
        != nullptr);
    QVERIFY(childObjectCount(fixture.view->contentItem(), QStringLiteral("shortcutDelegate")) > 0);

    const QStringList shortcutTexts
        = childTexts(fixture.view->contentItem(), QStringLiteral("shortcutKeycap"));
    const QStringList actionTexts
        = childTexts(fixture.view->contentItem(), QStringLiteral("shortcutActionLabel"));
    const QStringList categoryTitles
        = childTitles(fixture.view->contentItem(), QStringLiteral("shortcutCategoryHeader"));
    const QString actionTextDebug = actionTexts.join(QLatin1String(" | "));
    QVERIFY2(actionTexts.contains(QStringLiteral("Move to Trash")), qPrintable(actionTextDebug));
    QVERIFY2(
        actionTexts.contains(QStringLiteral("Delete Permanently")), qPrintable(actionTextDebug));
    QVERIFY(categoryTitles.contains(QStringLiteral("File")));
    QVERIFY(categoryTitles.contains(QStringLiteral("Navigation")));
    QVERIFY(categoryTitles.contains(QStringLiteral("View")));
    QVERIFY(categoryTitles.contains(QStringLiteral("Panels")));
    QVERIFY(categoryTitles.contains(QStringLiteral("Window")));
    QVERIFY(categoryTitles.contains(QStringLiteral("Settings")));
    QVERIFY(categoryTitles.contains(QStringLiteral("Help")));
    QVERIFY(shortcutTexts.contains(QStringLiteral("F11")));

    const QString allRowText
        = (shortcutTexts + actionTexts + categoryTitles).join(QLatin1Char('\n'));
    QVERIFY(!allRowText.contains(QStringLiteral("Ctrl+wheel")));
    QVERIFY(!allRowText.contains(QStringLiteral("Drag")));
    QVERIFY(!allRowText.contains(QStringLiteral("Esc")));
}

QTEST_MAIN(TestShortcutHelpDialog)

#include "test_shortcuthelpdialog.moc"
