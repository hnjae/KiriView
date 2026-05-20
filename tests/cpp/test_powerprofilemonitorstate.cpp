// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/powerprofilemonitorstate.h"

#include <QDBusVariant>
#include <QObject>
#include <QTest>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <optional>

class TestPowerProfileMonitorState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void portalValuesUnwrapDBusVariants();
    void powerSaverValueOwnsCanonicalStateChanges();
    void propertiesChangedAppliesPowerSaverProperty();
    void propertiesChangedRequestsRefreshForInvalidation();
    void refreshReplyFallsBackToDisabledWhenUnreadable();
};

void TestPowerProfileMonitorState::portalValuesUnwrapDBusVariants()
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

void TestPowerProfileMonitorState::powerSaverValueOwnsCanonicalStateChanges()
{
    KiriView::PowerProfileMonitorState state;
    QVERIFY(!state.powerSaverEnabled());

    KiriView::PowerProfileMonitorTransition transition = state.applyPowerSaverValue(true);
    QVERIFY(transition.powerSaverChanged);
    QVERIFY(!transition.refreshRequested);
    QVERIFY(state.powerSaverEnabled());

    transition = state.applyPowerSaverValue(true);
    QVERIFY(!transition.powerSaverChanged);
    QVERIFY(!transition.refreshRequested);
    QVERIFY(state.powerSaverEnabled());

    transition = state.applyPowerSaverValue(false);
    QVERIFY(transition.powerSaverChanged);
    QVERIFY(!state.powerSaverEnabled());
}

void TestPowerProfileMonitorState::propertiesChangedAppliesPowerSaverProperty()
{
    KiriView::PowerProfileMonitorState state;
    QVariantMap changedProperties;
    changedProperties.insert(
        QStringLiteral("power-saver-enabled"), QVariant::fromValue(QDBusVariant(QVariant(true))));

    KiriView::PowerProfileMonitorTransition transition = state.applyPropertiesChanged(
        QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), changedProperties, {});
    QVERIFY(transition.powerSaverChanged);
    QVERIFY(!transition.refreshRequested);
    QVERIFY(state.powerSaverEnabled());

    changedProperties.insert(QStringLiteral("power-saver-enabled"), QVariantMap());
    transition = state.applyPropertiesChanged(
        QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"), changedProperties, {});
    QVERIFY(transition.powerSaverChanged);
    QVERIFY(!state.powerSaverEnabled());

    changedProperties.insert(QStringLiteral("power-saver-enabled"), true);
    transition = state.applyPropertiesChanged(
        QStringLiteral("org.freedesktop.portal.Other"), changedProperties, {});
    QVERIFY(!transition.powerSaverChanged);
    QVERIFY(!transition.refreshRequested);
    QVERIFY(!state.powerSaverEnabled());
}

void TestPowerProfileMonitorState::propertiesChangedRequestsRefreshForInvalidation()
{
    KiriView::PowerProfileMonitorState state;
    KiriView::PowerProfileMonitorTransition transition
        = state.applyPropertiesChanged(QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"),
            {}, { QStringLiteral("power-saver-enabled") });
    QVERIFY(!transition.powerSaverChanged);
    QVERIFY(transition.refreshRequested);

    QVariantMap changedProperties;
    changedProperties.insert(QStringLiteral("power-saver-enabled"), true);
    transition
        = state.applyPropertiesChanged(QStringLiteral("org.freedesktop.portal.PowerProfileMonitor"),
            changedProperties, { QStringLiteral("power-saver-enabled") });
    QVERIFY(transition.powerSaverChanged);
    QVERIFY(!transition.refreshRequested);
    QVERIFY(state.powerSaverEnabled());
}

void TestPowerProfileMonitorState::refreshReplyFallsBackToDisabledWhenUnreadable()
{
    KiriView::PowerProfileMonitorState state;
    QVERIFY(state.applyRefreshReplyArguments(QVariantList { true }).powerSaverChanged);
    QVERIFY(state.powerSaverEnabled());

    KiriView::PowerProfileMonitorTransition transition = state.applyRefreshReplyArguments({});
    QVERIFY(transition.powerSaverChanged);
    QVERIFY(!transition.refreshRequested);
    QVERIFY(!state.powerSaverEnabled());

    state.applyPowerSaverValue(true);
    transition = state.applyRefreshReplyArguments(QVariantList { QVariantMap() });
    QVERIFY(transition.powerSaverChanged);
    QVERIFY(!state.powerSaverEnabled());
}

QTEST_GUILESS_MAIN(TestPowerProfileMonitorState)

#include "test_powerprofilemonitorstate.moc"
