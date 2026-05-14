// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <QVariant>

class TestToastNotification : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showActivatesNotification();
    void repeatedShowReplaysEntranceAnimation();
    void showWithDifferentScopeReplacesCurrentToast();
    void dismissIfScopeClearsBoundaryToast();
    void dismissIfScopeIgnoresGeneralToast();
    void dismissIsIdempotent();
};

namespace {
void addEnvironmentImportPaths(QQmlEngine &engine)
{
    const QString paths = qEnvironmentVariable("NIXPKGS_QML_SEARCH_PATHS");
    for (const QString &path : paths.split(QLatin1Char(':'), Qt::SkipEmptyParts)) {
        engine.addImportPath(path);
    }
}

QObject *createNotification(QQmlEngine &engine)
{
    addEnvironmentImportPaths(engine);

    QQmlComponent component(&engine,
        QUrl::fromLocalFile(
            QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../../src/qml/ToastNotification.qml")));

    if (component.isError()) {
        qWarning().noquote() << component.errorString();
    }

    QObject *object = component.create();
    if (object == nullptr) {
        qWarning().noquote() << component.errorString();
    }

    return object;
}

void showToastNotification(QObject &object, const QString &message, const QString &scope)
{
    QVERIFY(
        QMetaObject::invokeMethod(&object, "show", Q_ARG(QVariant, QVariant::fromValue(message)),
            Q_ARG(QVariant, QVariant::fromValue(scope))));
}

void dismissToastNotification(QObject &object)
{
    QVERIFY(QMetaObject::invokeMethod(&object, "dismiss"));
}

void dismissToastNotificationIfScope(QObject &object, const QString &scope)
{
    QVERIFY(QMetaObject::invokeMethod(
        &object, "dismissIfScope", Q_ARG(QVariant, QVariant::fromValue(scope))));
}

void compareClearedNotification(QObject &object)
{
    QCOMPARE(object.property("active").toBool(), false);
    QCOMPARE(object.property("message").toString(), QString());
    QCOMPARE(object.property("scope").toString(), QString());
}
}

void TestToastNotification::showActivatesNotification()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    showToastNotification(
        *notification, QStringLiteral("First image"), QStringLiteral("image-boundary"));

    QCOMPARE(notification->property("active").toBool(), true);
    QCOMPARE(notification->property("message").toString(), QStringLiteral("First image"));
    QCOMPARE(notification->property("scope").toString(), QStringLiteral("image-boundary"));
}

void TestToastNotification::repeatedShowReplaysEntranceAnimation()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    const QString message = QStringLiteral("Last image");
    const QString scope = QStringLiteral("image-boundary");
    const int initialReplayCount = notification->property("_entranceAnimationReplayCount").toInt();

    showToastNotification(*notification, message, scope);
    QCOMPARE(
        notification->property("_entranceAnimationReplayCount").toInt(), initialReplayCount + 1);

    showToastNotification(*notification, message, scope);
    QCOMPARE(
        notification->property("_entranceAnimationReplayCount").toInt(), initialReplayCount + 2);
    QCOMPARE(notification->property("active").toBool(), true);
    QCOMPARE(notification->property("message").toString(), message);
    QCOMPARE(notification->property("scope").toString(), scope);
}

void TestToastNotification::showWithDifferentScopeReplacesCurrentToast()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    showToastNotification(
        *notification, QStringLiteral("First image"), QStringLiteral("image-boundary"));
    showToastNotification(
        *notification, QStringLiteral("Could not delete file"), QStringLiteral("general"));

    QCOMPARE(notification->property("active").toBool(), true);
    QCOMPARE(notification->property("message").toString(), QStringLiteral("Could not delete file"));
    QCOMPARE(notification->property("scope").toString(), QStringLiteral("general"));
}

void TestToastNotification::dismissIfScopeClearsBoundaryToast()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    showToastNotification(
        *notification, QStringLiteral("First image"), QStringLiteral("image-boundary"));
    dismissToastNotificationIfScope(*notification, QStringLiteral("image-boundary"));

    compareClearedNotification(*notification);
}

void TestToastNotification::dismissIfScopeIgnoresGeneralToast()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    showToastNotification(
        *notification, QStringLiteral("Could not delete file"), QStringLiteral("general"));
    dismissToastNotificationIfScope(*notification, QStringLiteral("image-boundary"));

    QCOMPARE(notification->property("active").toBool(), true);
    QCOMPARE(notification->property("message").toString(), QStringLiteral("Could not delete file"));
    QCOMPARE(notification->property("scope").toString(), QStringLiteral("general"));
}

void TestToastNotification::dismissIsIdempotent()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    dismissToastNotification(*notification);
    showToastNotification(
        *notification, QStringLiteral("First image"), QStringLiteral("image-boundary"));
    dismissToastNotification(*notification);
    dismissToastNotification(*notification);

    compareClearedNotification(*notification);
}

QTEST_MAIN(TestToastNotification)

#include "test_toastnotification.moc"
