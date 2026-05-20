// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/shortcuthelpmodel.h"

#include <QAbstractItemModel>
#include <QList>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QTest>

namespace {
constexpr int actionIdRole = Qt::UserRole + 1;
constexpr int actionNameRole = Qt::UserRole + 2;
constexpr int actionTextRole = Qt::UserRole + 3;
constexpr int shortcutTextRole = Qt::UserRole + 4;

KiriView::ApplicationActions::ShortcutHelpRow row(
    int actionId, const QString &actionName, const QString &actionText, const QString &shortcutText)
{
    return KiriView::ApplicationActions::ShortcutHelpRow {
        actionId,
        actionName,
        actionText,
        shortcutText,
    };
}
}

class TestShortcutHelpModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rowsComeFromRuntimeProvider();
    void changedRowDataUpdatesOnlyMatchingRow();
    void changedRowIdentityResetsModel();
};

void TestShortcutHelpModel::rowsComeFromRuntimeProvider()
{
    const QList<KiriView::ApplicationActions::ShortcutHelpRow> rows {
        row(14, QStringLiteral("view_rotate_clockwise"), QStringLiteral("Rotate Clockwise"),
            QStringLiteral("Ctrl+R")),
    };
    KiriView::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, actionIdRole).toInt(), 14);
    QCOMPARE(model.data(index, actionNameRole).toString(), QStringLiteral("view_rotate_clockwise"));
    QCOMPARE(model.data(index, actionTextRole).toString(), QStringLiteral("Rotate Clockwise"));
    QCOMPARE(model.data(index, shortcutTextRole).toString(), QStringLiteral("Ctrl+R"));
}

void TestShortcutHelpModel::changedRowDataUpdatesOnlyMatchingRow()
{
    QList<KiriView::ApplicationActions::ShortcutHelpRow> rows {
        row(0, QStringLiteral("file_open"), QStringLiteral("Open"), QStringLiteral("Ctrl+O")),
        row(14, QStringLiteral("view_rotate_clockwise"), QStringLiteral("Rotate Clockwise"),
            QStringLiteral("Ctrl+R")),
    };
    KiriView::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    rows[1].shortcutText = QStringLiteral("Alt+R");
    model.handleRowsChanged();

    QCOMPARE(dataChangedSpy.count(), 1);
    const QModelIndex changedIndex = dataChangedSpy.at(0).at(0).toModelIndex();
    QCOMPARE(changedIndex.row(), 1);
    QCOMPARE(model.data(model.index(1, 0), shortcutTextRole).toString(), QStringLiteral("Alt+R"));
}

void TestShortcutHelpModel::changedRowIdentityResetsModel()
{
    QList<KiriView::ApplicationActions::ShortcutHelpRow> rows {
        row(0, QStringLiteral("file_open"), QStringLiteral("Open"), QStringLiteral("Ctrl+O")),
    };
    KiriView::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    rows.clear();
    model.handleRowsChanged();

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 0);
}

QTEST_MAIN(TestShortcutHelpModel)

#include "test_shortcuthelpmodel.moc"
