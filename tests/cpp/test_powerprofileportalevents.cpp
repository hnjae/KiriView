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
void compareEvent(const KiriView::PowerProfileMonitorEvent &event,
    KiriView::PowerProfileMonitorEventKind kind, bool powerSaverEnabled = false)
{
    QCOMPARE(static_cast<int>(event.kind), static_cast<int>(kind));
    QCOMPARE(event.powerSaverEnabled, powerSaverEnabled);
}
}

void TestPowerProfilePortalEvents::portalValuesUnwrapDBusVariants()
{
    std::optional<bool> value = KiriView::powerSaverEnabledFromPortalValue(QVariant(true));
    QVERIFY(value.has_value());
    QCOMPARE(*value, true);

    value = KiriView::powerSaverEnabledFromPortalValue(
        QVariant::fromValue(QDBusVariant(QVariant(false))));
    QVERIFY(value.has_value());
    QCOMPARE(*value, false);

    QVERIFY(!KiriView::powerSaverEnabledFromPortalValue(QVariant(QVariantMap())).has_value());
}

void TestPowerProfilePortalEvents::refreshReplyFallsBackToDisabledWhenUnreadable()
{
    compareEvent(KiriView::powerProfileMonitorEventFromRefreshReply(QVariantList { true }),
        KiriView::PowerProfileMonitorEventKind::PowerSaverValue, true);
    compareEvent(KiriView::powerProfileMonitorEventFromRefreshReply({}),
        KiriView::PowerProfileMonitorEventKind::PowerSaverValue, false);
    compareEvent(KiriView::powerProfileMonitorEventFromRefreshReply(QVariantList { QVariantMap() }),
        KiriView::PowerProfileMonitorEventKind::PowerSaverValue, false);
}

void TestPowerProfilePortalEvents::propertiesChangedAppliesPowerSaverProperty()
{
    QVariantMap changedProperties;
    changedProperties.insert(
        QStringLiteral("power-saver-enabled"), QVariant::fromValue(QDBusVariant(QVariant(true))));

    compareEvent(
        KiriView::powerProfileMonitorEventFromPropertiesChanged(
            QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), changedProperties, {}),
        KiriView::PowerProfileMonitorEventKind::PowerSaverValue, true);

    changedProperties.insert(QStringLiteral("power-saver-enabled"), QVariantMap());
    compareEvent(
        KiriView::powerProfileMonitorEventFromPropertiesChanged(
            QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), changedProperties, {}),
        KiriView::PowerProfileMonitorEventKind::PowerSaverValue, false);

    changedProperties.insert(QStringLiteral("power-saver-enabled"), true);
    compareEvent(KiriView::powerProfileMonitorEventFromPropertiesChanged(
                     QStringLiteral("org.freedesktop.portal.Other"), changedProperties, {}),
        KiriView::PowerProfileMonitorEventKind::Ignore);
}

void TestPowerProfilePortalEvents::propertiesChangedRequestsRefreshForInvalidation()
{
    compareEvent(KiriView::powerProfileMonitorEventFromPropertiesChanged(
                     QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), {},
                     { QStringLiteral("power-saver-enabled") }),
        KiriView::PowerProfileMonitorEventKind::PowerSaverInvalidated);

    QVariantMap changedProperties;
    changedProperties.insert(QStringLiteral("power-saver-enabled"), true);
    compareEvent(KiriView::powerProfileMonitorEventFromPropertiesChanged(
                     QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"),
                     changedProperties, { QStringLiteral("power-saver-enabled") }),
        KiriView::PowerProfileMonitorEventKind::PowerSaverValue, true);
}

QTEST_GUILESS_MAIN(TestPowerProfilePortalEvents)

#include "test_powerprofileportalevents.moc"
