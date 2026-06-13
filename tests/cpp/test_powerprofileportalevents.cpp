// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/powerprofileportalevents.h"

#include <QDBusVariant>
#include <QObject>
#include <QTest>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <optional>

class TestPowerProfilePortalEvents : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void portalValuesUnwrapDBusVariants();
    void refreshReplyFallsBackToDisabledWhenUnreadable();
    void propertiesChangedAppliesPowerSaverProperty();
    void propertiesChangedRequestsRefreshForInvalidation();
};

namespace {
void compareEvent(const kiriview::PowerProfileMonitorEvent &event,
    kiriview::PowerProfileMonitorEventKind kind, bool powerSaverEnabled = false)
{
    QCOMPARE(static_cast<int>(event.kind), static_cast<int>(kind));
    QCOMPARE(event.powerSaverEnabled, powerSaverEnabled);
}
}

void TestPowerProfilePortalEvents::portalValuesUnwrapDBusVariants()
{
    std::optional<bool> value = kiriview::powerSaverEnabledFromPortalValue(QVariant(true));
    QVERIFY(value.has_value());
    QCOMPARE(*value, true);

    value = kiriview::powerSaverEnabledFromPortalValue(
        QVariant::fromValue(QDBusVariant(QVariant(false))));
    QVERIFY(value.has_value());
    QCOMPARE(*value, false);

    QVERIFY(!kiriview::powerSaverEnabledFromPortalValue(QVariant(QVariantMap())).has_value());
}

void TestPowerProfilePortalEvents::refreshReplyFallsBackToDisabledWhenUnreadable()
{
    compareEvent(kiriview::powerProfileMonitorEventFromRefreshReply(QVariantList { true }),
        kiriview::PowerProfileMonitorEventKind::PowerSaverValue, true);
    compareEvent(kiriview::powerProfileMonitorEventFromRefreshReply({}),
        kiriview::PowerProfileMonitorEventKind::PowerSaverValue, false);
    compareEvent(kiriview::powerProfileMonitorEventFromRefreshReply(QVariantList { QVariantMap() }),
        kiriview::PowerProfileMonitorEventKind::PowerSaverValue, false);
}

void TestPowerProfilePortalEvents::propertiesChangedAppliesPowerSaverProperty()
{
    QVariantMap changedProperties;
    changedProperties.insert(
        QStringLiteral("power-saver-enabled"), QVariant::fromValue(QDBusVariant(QVariant(true))));

    compareEvent(
        kiriview::powerProfileMonitorEventFromPropertiesChanged(
            QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), changedProperties, {}),
        kiriview::PowerProfileMonitorEventKind::PowerSaverValue, true);

    changedProperties.insert(QStringLiteral("power-saver-enabled"), QVariantMap());
    compareEvent(
        kiriview::powerProfileMonitorEventFromPropertiesChanged(
            QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), changedProperties, {}),
        kiriview::PowerProfileMonitorEventKind::PowerSaverValue, false);

    changedProperties.insert(QStringLiteral("power-saver-enabled"), true);
    compareEvent(kiriview::powerProfileMonitorEventFromPropertiesChanged(
                     QStringLiteral("org.freedesktop.portal.Other"), changedProperties, {}),
        kiriview::PowerProfileMonitorEventKind::Ignore);
}

void TestPowerProfilePortalEvents::propertiesChangedRequestsRefreshForInvalidation()
{
    compareEvent(kiriview::powerProfileMonitorEventFromPropertiesChanged(
                     QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), {},
                     { QStringLiteral("power-saver-enabled") }),
        kiriview::PowerProfileMonitorEventKind::PowerSaverInvalidated);

    QVariantMap changedProperties;
    changedProperties.insert(QStringLiteral("power-saver-enabled"), true);
    compareEvent(kiriview::powerProfileMonitorEventFromPropertiesChanged(
                     QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"),
                     changedProperties, { QStringLiteral("power-saver-enabled") }),
        kiriview::PowerProfileMonitorEventKind::PowerSaverValue, true);
}

QTEST_GUILESS_MAIN(TestPowerProfilePortalEvents)

#include "test_powerprofileportalevents.moc"
