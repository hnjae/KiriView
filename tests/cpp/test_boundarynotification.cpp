// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <QVariant>

class TestBoundaryNotification : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showActivatesNotification();
    void showReplacesActiveMessage();
    void dismissClearsNotification();
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
            QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../../src/qml/BoundaryNotification.qml")));

    if (component.isError()) {
        qWarning().noquote() << component.errorString();
    }

    QObject *object = component.create();
    if (object == nullptr) {
        qWarning().noquote() << component.errorString();
    }

    return object;
}

void showBoundaryNotification(QObject &object, const QString &message)
{
    QVERIFY(
        QMetaObject::invokeMethod(&object, "show", Q_ARG(QVariant, QVariant::fromValue(message))));
}

void dismissBoundaryNotification(QObject &object)
{
    QVERIFY(QMetaObject::invokeMethod(&object, "dismiss"));
}
}

void TestBoundaryNotification::showActivatesNotification()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    showBoundaryNotification(*notification, QStringLiteral("First image"));

    QCOMPARE(notification->property("active").toBool(), true);
    QCOMPARE(notification->property("message").toString(), QStringLiteral("First image"));
}

void TestBoundaryNotification::showReplacesActiveMessage()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    showBoundaryNotification(*notification, QStringLiteral("First image"));
    showBoundaryNotification(*notification, QStringLiteral("First image"));
    showBoundaryNotification(*notification, QStringLiteral("Last image"));

    QCOMPARE(notification->property("active").toBool(), true);
    QCOMPARE(notification->property("message").toString(), QStringLiteral("Last image"));
}

void TestBoundaryNotification::dismissClearsNotification()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    showBoundaryNotification(*notification, QStringLiteral("First image"));
    dismissBoundaryNotification(*notification);

    QCOMPARE(notification->property("active").toBool(), false);
    QCOMPARE(notification->property("message").toString(), QString());
}

void TestBoundaryNotification::dismissIsIdempotent()
{
    QQmlEngine engine;
    QScopedPointer<QObject> notification(createNotification(engine));
    QVERIFY(!notification.isNull());

    dismissBoundaryNotification(*notification);
    dismissBoundaryNotification(*notification);

    QCOMPARE(notification->property("active").toBool(), false);
    QCOMPARE(notification->property("message").toString(), QString());
}

QTEST_MAIN(TestBoundaryNotification)

#include "test_boundarynotification.moc"
