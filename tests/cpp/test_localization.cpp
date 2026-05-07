// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewtext.h"
#include "kiriviewapplication.h"
#include "localization.h"

#include <KLocalizedString>
#include <QAction>
#include <QFile>
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
    void actionsUseKoreanCatalog();
    void statusMessagesUseKoreanCatalog();
    void qmlContextUsesKoreanCatalog();
    void desktopFileIncludesKoreanGenericName();
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

void TestLocalization::actionsUseKoreanCatalog()
{
    KiriViewApplication application;

    QAction *openAction = application.action(QStringLiteral("file_open"));
    QVERIFY(openAction != nullptr);
    QCOMPARE(openAction->text(), QStringLiteral("열기"));

    QAction *previousArchiveAction = application.action(QStringLiteral("go_previous_archive"));
    QVERIFY(previousArchiveAction != nullptr);
    QCOMPARE(previousArchiveAction->text(), QStringLiteral("이전 압축파일"));
}

void TestLocalization::statusMessagesUseKoreanCatalog()
{
    QCOMPARE(KiriView::imageViewText("Could not read the selected image data."),
        QStringLiteral("선택한 이미지 데이터를 읽을 수 없습니다."));
}

void TestLocalization::qmlContextUsesKoreanCatalog()
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
    QCOMPARE(object->property("openText").toString(), QStringLiteral("열기"));
}

void TestLocalization::desktopFileIncludesKoreanGenericName()
{
    QFile desktopFile(QStringLiteral(KIRIVIEW_TEST_DESKTOP_FILE));
    QVERIFY(desktopFile.open(QIODevice::ReadOnly));

    const QString desktopText = QString::fromUtf8(desktopFile.readAll());
    QVERIFY(desktopText.contains(QStringLiteral("GenericName[ko]=이미지 뷰어")));
}

QTEST_MAIN(TestLocalization)

#include "test_localization.moc"
