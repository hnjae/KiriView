// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationruntime.h"
#include "imageerrortext.h"
#include "kiriviewapplication.h"
#include "localization.h"

#include <KLocalizedString>
#include <QAction>
#include <QFile>
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
    void desktopFileIncludesTranslatedGenericName();
};

void TestLocalization::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KiriView::initializeLocalization();
    KLocalizedString::addDomainLocaleDir(
        QByteArrayLiteral("kiriview"), QStringLiteral(KIRIVIEW_TEST_LOCALE_DIR));
}

void TestLocalization::init() { KLocalizedString::setLanguages({ QStringLiteral("ko") }); }

void TestLocalization::cleanup() { KLocalizedString::clearLanguages(); }

void TestLocalization::actionsUseTestCatalog()
{
    KiriViewApplication application;

    QAction *openAction = application.action(QStringLiteral("file_open"));
    QVERIFY(openAction != nullptr);
    QCOMPARE(openAction->text(), QStringLiteral("__kiriview_test_open__"));

    QAction *previousArchiveAction = application.action(QStringLiteral("go_previous_archive"));
    QVERIFY(previousArchiveAction != nullptr);
    QCOMPARE(previousArchiveAction->text(), QStringLiteral("__kiriview_test_previous_archive__"));
}

void TestLocalization::statusMessagesUseTestCatalog()
{
    QCOMPARE(KiriView::imageErrorText(KiriView::ImageErrorTextId::ReadImageData),
        QStringLiteral("__kiriview_test_status_read_image_data__"));
}

void TestLocalization::applicationRuntimeSetsDesktopFileName()
{
    KiriView::initializeApplicationRuntime();

    QCOMPARE(QGuiApplication::desktopFileName(), QStringLiteral("io.github.hnjae.KiriView"));
}

void TestLocalization::qmlContextUsesTestCatalog()
{
    QQmlApplicationEngine engine;
    KiriView::setupLocalizedContext(engine);

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

void TestLocalization::desktopFileIncludesTranslatedGenericName()
{
    QFile desktopFile(QStringLiteral(KIRIVIEW_TEST_DESKTOP_FILE));
    QVERIFY(desktopFile.open(QIODevice::ReadOnly));

    const QString desktopText = QString::fromUtf8(desktopFile.readAll());
    QVERIFY(desktopText.contains(QStringLiteral("GenericName[ko]=__kiriview_test_generic_name__")));
}

QTEST_MAIN(TestLocalization)

#include "test_localization.moc"
