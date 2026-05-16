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
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
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
    void ctrlMTogglesMenubarThroughAction();
    void ctrlMIgnoredWhileHelpDialogOpen();
};

namespace {
struct ImageShortcutsFixture {
    std::unique_ptr<QQuickView> view;
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

QString fixtureQml()
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

    function menuPresentation() {
        return application.menuPresentation;
    }

    Component.onCompleted: forceActiveFocus()

    KiriViewApplication {
        id: application

        objectName: "application"
    }

    KiriImageDocument {
        id: imageDocument
    }

    QtObject {
        id: imageViewport

        property bool imageHorizontallyPannable: false
        property bool imagePannable: false
        property real viewportHeight: 100
        property real viewportWidth: 100

        function panBy(deltaX, deltaY) {
            return false;
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
            return false;
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
        .arg(qmlSourceImport());
}

ImageShortcutsFixture createFixture()
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
        fixtureQml().toUtf8(), QUrl(QStringLiteral("memory:test_imageshortcuts.qml")));
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

int menuPresentation(QObject *root)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        root, "menuPresentation", Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked ? result.toInt() : -1;
}

void pressCtrlM(QQuickView *view)
{
    QTest::keyClick(view, Qt::Key_M, Qt::ControlModifier);
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
