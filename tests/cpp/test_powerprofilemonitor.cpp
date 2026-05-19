// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powerprofilemonitor.h"

#include <QDBusVariant>
#include <QMetaObject>
#include <QObject>
#include <QTest>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <utility>
#include <vector>

namespace {
struct FakePowerProfilePortal {
    QVariantList readArguments;
    int readCount = 0;
    int subscriptionCount = 0;
    QObject *subscriber = nullptr;

    KiriView::PowerProfileMonitorRuntime runtime()
    {
        return KiriView::PowerProfileMonitorRuntime {
            [this]() {
                ++readCount;
                return readArguments;
            },
            [this](QObject *receiver) {
                ++subscriptionCount;
                subscriber = receiver;
            },
        };
    }

    bool emitPropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties,
        const QStringList &invalidatedProperties) const
    {
        if (subscriber == nullptr) {
            return false;
        }

        return QMetaObject::invokeMethod(subscriber, "handlePropertiesChanged",
            Qt::DirectConnection, Q_ARG(QString, interfaceName),
            Q_ARG(QVariantMap, changedProperties), Q_ARG(QStringList, invalidatedProperties));
    }
};
}

class TestPowerProfileMonitor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialRefreshReadsPortalAndSubscribes();
    void changedPropertyUpdatesCanonicalStateWithoutRefresh();
    void invalidatedPropertyRequestsRefreshThroughRuntime();
    void runtimeDefaultsFillMissingEffectsAndPreserveOverrides();
};

void TestPowerProfileMonitor::initialRefreshReadsPortalAndSubscribes()
{
    FakePowerProfilePortal portal;
    portal.readArguments = { QVariant::fromValue(QDBusVariant(QVariant(true))) };
    std::vector<bool> changes;

    KiriView::PowerProfileMonitor monitor(
        nullptr, [&changes](bool enabled) { changes.push_back(enabled); }, portal.runtime());

    QCOMPARE(portal.readCount, 1);
    QCOMPARE(portal.subscriptionCount, 1);
    QCOMPARE(portal.subscriber, &monitor);
    QVERIFY(monitor.powerSaverEnabled());
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.at(0), true);
}

void TestPowerProfileMonitor::changedPropertyUpdatesCanonicalStateWithoutRefresh()
{
    FakePowerProfilePortal portal;
    std::vector<bool> changes;
    KiriView::PowerProfileMonitor monitor(
        nullptr, [&changes](bool enabled) { changes.push_back(enabled); }, portal.runtime());

    QVariantMap changedProperties;
    changedProperties.insert(
        QStringLiteral("power-saver-enabled"), QVariant::fromValue(QDBusVariant(QVariant(true))));

    QVERIFY(portal.emitPropertiesChanged(
        QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), changedProperties, {}));

    QCOMPARE(portal.readCount, 1);
    QVERIFY(monitor.powerSaverEnabled());
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.at(0), true);
}

void TestPowerProfileMonitor::invalidatedPropertyRequestsRefreshThroughRuntime()
{
    FakePowerProfilePortal portal;
    std::vector<bool> changes;
    KiriView::PowerProfileMonitor monitor(
        nullptr, [&changes](bool enabled) { changes.push_back(enabled); }, portal.runtime());

    portal.readArguments = { true };
    QVERIFY(
        portal.emitPropertiesChanged(QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"),
            {}, { QStringLiteral("power-saver-enabled") }));

    QCOMPARE(portal.readCount, 2);
    QVERIFY(monitor.powerSaverEnabled());
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.at(0), true);

    portal.readArguments = {};
    QVERIFY(
        portal.emitPropertiesChanged(QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"),
            {}, { QStringLiteral("power-saver-enabled") }));

    QCOMPARE(portal.readCount, 3);
    QVERIFY(!monitor.powerSaverEnabled());
    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(1), false);
}

void TestPowerProfileMonitor::runtimeDefaultsFillMissingEffectsAndPreserveOverrides()
{
    int readCount = 0;
    KiriView::PowerProfileMonitorRuntime runtime;
    runtime.readPowerSaverEnabled = [&readCount]() {
        ++readCount;
        return QVariantList { true };
    };

    KiriView::PowerProfileMonitorRuntime resolved
        = KiriView::powerProfileMonitorRuntimeWithDefaults(std::move(runtime));
    QVERIFY(resolved.readPowerSaverEnabled);
    QVERIFY(resolved.subscribePropertiesChanged);
    const QVariantList arguments = resolved.readPowerSaverEnabled();
    QCOMPARE(arguments.size(), 1);
    QCOMPARE(arguments.first().toBool(), true);
    QCOMPARE(readCount, 1);
}

QTEST_GUILESS_MAIN(TestPowerProfileMonitor)

#include "test_powerprofilemonitor.moc"
