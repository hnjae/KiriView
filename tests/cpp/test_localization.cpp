// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationruntime.h"
#include "facade/kiriviewapplication.h"
#include "localization/imageerrortext.h"
#include "localization/localization.h"

#include <KLocalizedString>
#include <QAction>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QScopedPointer>
#include <QStandardPaths>
#include <QTest>

class TestLocalization : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void actionsUseTestCatalog();
    void statusMessagesUseTestCatalog();
    void applicationRuntimeSetsDesktopFileName();
    void qmlContextUsesTestCatalog();
};

void TestLocalization::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    kiriview::initializeLocalization();
    KLocalizedString::addDomainLocaleDir(
        QByteArrayLiteral("kiriview"), QStringLiteral(KIRIVIEW_TEST_LOCALE_DIR));
}

void TestLocalization::init() { KLocalizedString::setLanguages({ QStringLiteral("ko") }); }

void TestLocalization::cleanup() { KLocalizedString::clearLanguages(); }

void TestLocalization::actionsUseTestCatalog()
{
    KiriViewApplication application;

    QAction* openAction = application.action(QStringLiteral("file_open"));
    QVERIFY(openAction != nullptr);
    QCOMPARE(openAction->text(), QStringLiteral("__kiriview_test_open__"));

    QAction* previousArchiveAction = application.action(QStringLiteral("go_previous_archive"));
    QVERIFY(previousArchiveAction != nullptr);
    QCOMPARE(previousArchiveAction->text(), QStringLiteral("__kiriview_test_previous_archive__"));
}

void TestLocalization::statusMessagesUseTestCatalog()
{
    QCOMPARE(kiriview::imageErrorText(kiriview::ImageErrorTextId::ReadImageData),
        QStringLiteral("__kiriview_test_status_read_image_data__"));
}

void TestLocalization::applicationRuntimeSetsDesktopFileName()
{
    kiriview::initializeApplicationRuntime();

    QCOMPARE(QGuiApplication::desktopFileName(), QStringLiteral("org.hnjae.kiriview"));
}

void TestLocalization::qmlContextUsesTestCatalog()
{
    QQmlApplicationEngine engine;
    kiriview::setupLocalizedContext(engine);

    QQmlComponent component(&engine);
    component.setData(
        R"(
            import QtQml
            import org.kde.ki18n

            QtObject {
                property string openText: KI18n.i18n("Open")
            }
        )",
        QUrl());

    if (component.isError()) {
        QFAIL(qPrintable(component.errorString()));
    }

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));
    QCOMPARE(object->property("openText").toString(), QStringLiteral("__kiriview_test_open__"));
}

QTEST_MAIN(TestLocalization)

#include "test_localization.moc"
