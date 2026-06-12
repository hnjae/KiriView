// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/shortcutroutemodel.h"

#include "application/applicationshortcutpolicy.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QTest>
#include <QVariantList>

namespace {
using ActionId = KiriView::ApplicationActions::ActionId;

QVariantList actionIdVariants(const QList<ActionId> &actionIds)
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
    KiriView::ApplicationActions::ShortcutRouteModel model;
    const QList<KiriView::ApplicationActions::ApplicationShortcutRoute> &routes
        = KiriView::ApplicationActions::shortcutRoutes();

    QCOMPARE(model.rowCount(), static_cast<int>(routes.size()));
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);

    const QHash<int, QByteArray> roles = model.roleNames();
    QCOMPARE(roles.value(KiriView::ApplicationActions::ShortcutRouteModel::ActionIdsRole),
        QByteArrayLiteral("actionIds"));
    QCOMPARE(roles.value(KiriView::ApplicationActions::ShortcutRouteModel::ActivationScopeRole),
        QByteArrayLiteral("activationScope"));
    QCOMPARE(roles.value(KiriView::ApplicationActions::ShortcutRouteModel::ShortcutScopeRole),
        QByteArrayLiteral("shortcutScope"));

    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0);
        const KiriView::ApplicationActions::ApplicationShortcutRoute &route = routes.at(row);
        QCOMPARE(model.data(index, KiriView::ApplicationActions::ShortcutRouteModel::ActionIdsRole)
                     .toList(),
            actionIdVariants(route.actionIds));
        QCOMPARE(
            model.data(index, KiriView::ApplicationActions::ShortcutRouteModel::ActivationScopeRole)
                .toInt(),
            static_cast<int>(route.activationScope));
        QCOMPARE(
            model.data(index, KiriView::ApplicationActions::ShortcutRouteModel::ShortcutScopeRole)
                .toInt(),
            static_cast<int>(route.shortcutScope));
    }
}

void TestShortcutRouteModel::invalidIndexesReturnEmptyData()
{
    KiriView::ApplicationActions::ShortcutRouteModel model;

    QVERIFY(
        !model.data(QModelIndex(), KiriView::ApplicationActions::ShortcutRouteModel::ActionIdsRole)
            .isValid());
    QVERIFY(!model
            .data(model.index(model.rowCount(), 0),
                KiriView::ApplicationActions::ShortcutRouteModel::ActionIdsRole)
            .isValid());
    QVERIFY(!model.data(model.index(0, 0), Qt::DisplayRole).isValid());
}

QTEST_GUILESS_MAIN(TestShortcutRouteModel)

#include "test_shortcutroutemodel.moc"
