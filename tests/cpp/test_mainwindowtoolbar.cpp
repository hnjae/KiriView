// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationruntime.h"
#include "facade/imageactionavailability.h"
#include "facade/imageshortcutnavigationpolicy.h"
#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedocument.h"
#include "facade/kiriimageview.h"
#include "facade/kirivideodocument.h"
#include "facade/kiriviewapplication.h"
#include "facade/menuaccesskeyrouter.h"
#include "localization/localization.h"

#include <KLocalizedQmlContext>
#include <QDir>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTest>
#include <QUrl>
#include <QtQml/qqml.h>
#include <memory>

class TestMainWindowToolBar : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void startupCreatesOneVisibleToolbarWithDisabledMediaControls();
    void fullscreenReusesSingleToolbarAndHidesApplicationMenuButton();
};

namespace {
struct MainWindowFixture {
    std::unique_ptr<QQmlApplicationEngine> engine;
    QQuickWindow *window = nullptr;
    QString errorString;

    bool isValid() const { return engine != nullptr && window != nullptr; }
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
    qmlRegisterType<ImageShortcutNavigationPolicy>(
        "io.github.hnjae.kiriview", 1, 0, "ImageShortcutNavigationPolicy");
    qmlRegisterType<KiriDocumentSession>("io.github.hnjae.kiriview", 1, 0, "KiriDocumentSession");
    qmlRegisterType<KiriImageDocument>("io.github.hnjae.kiriview", 1, 0, "KiriImageDocument");
    qmlRegisterType<KiriImageView>("io.github.hnjae.kiriview", 1, 0, "KiriImageView");
    qmlRegisterType<KiriVideoDocument>("io.github.hnjae.kiriview", 1, 0, "KiriVideoDocument");
    qmlRegisterType<MenuAccessKeyRouter>("io.github.hnjae.kiriview", 1, 0, "MenuAccessKeyRouter");
    registered = true;
}

QUrl mainQmlUrl()
{
    return QUrl::fromLocalFile(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml/Main.qml")));
}

QList<QQuickItem *> controlToolBars(QObject *root)
{
    QList<QQuickItem *> toolbars;
    const QList<QQuickItem *> items
        = root->findChildren<QQuickItem *>(QString(), Qt::FindChildrenRecursively);
    for (QQuickItem *item : items) {
        if (item->inherits("QQuickToolBar")) {
            toolbars.append(item);
        }
    }
    return toolbars;
}

QList<QQuickItem *> visibleItemsByObjectName(QObject *root, const QString &objectName)
{
    QList<QQuickItem *> visibleItems;
    const QList<QQuickItem *> items
        = root->findChildren<QQuickItem *>(objectName, Qt::FindChildrenRecursively);
    for (QQuickItem *item : items) {
        if (item->isVisible()) {
            visibleItems.append(item);
        }
    }
    return visibleItems;
}

QQuickItem *findQuickItem(QObject *root, const QString &objectName)
{
    return root->findChild<QQuickItem *>(objectName, Qt::FindChildrenRecursively);
}

MainWindowFixture createMainWindowFixture()
{
    MainWindowFixture fixture;
    registerKiriViewQmlTypes();
    fixture.engine = std::make_unique<QQmlApplicationEngine>();
    addEnvironmentImportPaths(*fixture.engine);
    fixture.engine->addImportPath(QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
            .absoluteFilePath(QStringLiteral("../../src/qml")));
    KLocalization::setupLocalizedContext(fixture.engine.get());

    fixture.engine->load(mainQmlUrl());
    if (fixture.engine->rootObjects().isEmpty()) {
        fixture.errorString = QStringLiteral("Main.qml did not create a root object");
        return fixture;
    }

    fixture.window = qobject_cast<QQuickWindow *>(fixture.engine->rootObjects().constFirst());
    if (fixture.window == nullptr) {
        fixture.errorString = QStringLiteral("Main.qml root object is not a QQuickWindow");
        return fixture;
    }

    if (!QTest::qWaitForWindowExposed(fixture.window)) {
        fixture.errorString = QStringLiteral("main window was not exposed");
        return fixture;
    }

    return fixture;
}
}

void TestMainWindowToolBar::initTestCase()
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}

void TestMainWindowToolBar::init()
{
    QTest::failOnWarning(QRegularExpression(
        QStringLiteral(".*Created graphical object was not placed in the graphics scene.*")));
}

void TestMainWindowToolBar::startupCreatesOneVisibleToolbarWithDisabledMediaControls()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));

    QTRY_COMPARE(controlToolBars(fixture.window).size(), 1);
    QQuickItem *toolbar = controlToolBars(fixture.window).constFirst();
    QVERIFY(toolbar->isVisible());
    QCOMPARE(toolbar->objectName(), QStringLiteral("mainImageToolBar"));

    QQuickItem *leftPageButton
        = findQuickItem(fixture.window, QStringLiteral("leftPageNavigationButton"));
    QQuickItem *rightPageButton
        = findQuickItem(fixture.window, QStringLiteral("rightPageNavigationButton"));
    QVERIFY(leftPageButton != nullptr);
    QVERIFY(rightPageButton != nullptr);
    QVERIFY(!leftPageButton->isEnabled());
    QVERIFY(!rightPageButton->isEnabled());

    const QList<QQuickItem *> visibleApplicationMenuButtons
        = visibleItemsByObjectName(fixture.window, QStringLiteral("toolbarApplicationMenuButton"));
    QCOMPARE(visibleApplicationMenuButtons.size(), 1);
    QVERIFY(visibleApplicationMenuButtons.constFirst()->isEnabled());
}

void TestMainWindowToolBar::fullscreenReusesSingleToolbarAndHidesApplicationMenuButton()
{
    MainWindowFixture fixture = createMainWindowFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_COMPARE(controlToolBars(fixture.window).size(), 1);
    QQuickItem *windowedToolbar = controlToolBars(fixture.window).constFirst();

    fixture.window->setVisibility(QWindow::FullScreen);
    QTRY_COMPARE(fixture.window->visibility(), QWindow::FullScreen);

    QTRY_COMPARE(controlToolBars(fixture.window).size(), 1);
    QCOMPARE(controlToolBars(fixture.window).constFirst(), windowedToolbar);
    QVERIFY(windowedToolbar->isVisible());

    const QList<QQuickItem *> visibleApplicationMenuButtons
        = visibleItemsByObjectName(fixture.window, QStringLiteral("toolbarApplicationMenuButton"));
    QVERIFY(visibleApplicationMenuButtons.isEmpty());
}

QTEST_MAIN(TestMainWindowToolBar)

#include "test_mainwindowtoolbar.moc"
