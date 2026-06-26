// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/shortcutroutemodel.h"

#include "application/applicationshortcutpolicy.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QTest>
#include <QVariantList>

namespace {
using ActionId = kiriview::ApplicationActions::ActionId;

QVariantList actionIdVariants(const QList<ActionId>& actionIds)
{
    QVariantList variants;
    variants.reserve(actionIds.size());

    for (ActionId actionId : actionIds) {
        variants.push_back(static_cast<int>(actionId));
    }

    return variants;
}
}

class TestShortcutRouteModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void routeRowsExposeApplicationShortcutPolicy();
    void invalidIndexesReturnEmptyData();
};

void TestShortcutRouteModel::routeRowsExposeApplicationShortcutPolicy()
{
    kiriview::ApplicationActions::ShortcutRouteModel model;
    const QList<kiriview::ApplicationActions::ApplicationShortcutRoute>& routes
        = kiriview::ApplicationActions::shortcutRoutes();

    QCOMPARE(model.rowCount(), static_cast<int>(routes.size()));
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);

    const QHash<int, QByteArray> roles = model.roleNames();
    QCOMPARE(roles.value(kiriview::ApplicationActions::ShortcutRouteModel::ActionIdsRole),
        QByteArrayLiteral("actionIds"));
    QCOMPARE(roles.value(kiriview::ApplicationActions::ShortcutRouteModel::ActivationScopeRole),
        QByteArrayLiteral("activationScope"));
    QCOMPARE(roles.value(kiriview::ApplicationActions::ShortcutRouteModel::ShortcutScopeRole),
        QByteArrayLiteral("shortcutScope"));

    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0);
        const kiriview::ApplicationActions::ApplicationShortcutRoute& route = routes.at(row);
        QCOMPARE(model.data(index, kiriview::ApplicationActions::ShortcutRouteModel::ActionIdsRole)
                     .toList(),
            actionIdVariants(route.actionIds));
        QCOMPARE(
            model.data(index, kiriview::ApplicationActions::ShortcutRouteModel::ActivationScopeRole)
                .toInt(),
            static_cast<int>(route.activationScope));
        QCOMPARE(
            model.data(index, kiriview::ApplicationActions::ShortcutRouteModel::ShortcutScopeRole)
                .toInt(),
            static_cast<int>(route.shortcutScope));
    }
}

void TestShortcutRouteModel::invalidIndexesReturnEmptyData()
{
    kiriview::ApplicationActions::ShortcutRouteModel model;

    QVERIFY(
        !model.data(QModelIndex(), kiriview::ApplicationActions::ShortcutRouteModel::ActionIdsRole)
            .isValid());
    QVERIFY(!model
            .data(model.index(model.rowCount(), 0),
                kiriview::ApplicationActions::ShortcutRouteModel::ActionIdsRole)
            .isValid());
    QVERIFY(!model.data(model.index(0, 0), Qt::DisplayRole).isValid());
}

QTEST_GUILESS_MAIN(TestShortcutRouteModel)

#include "test_shortcutroutemodel.moc"
