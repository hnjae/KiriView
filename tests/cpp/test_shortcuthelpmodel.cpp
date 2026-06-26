// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/shortcuthelpmodel.h"

#include <QAbstractItemModel>
#include <QList>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTest>

namespace {
constexpr int actionIdRole = Qt::UserRole + 1;
constexpr int actionNameRole = Qt::UserRole + 2;
constexpr int actionTextRole = Qt::UserRole + 3;
constexpr int shortcutTextRole = Qt::UserRole + 4;
constexpr int categoryKeyRole = Qt::UserRole + 5;
constexpr int categoryTextRole = Qt::UserRole + 6;
constexpr int categoryFirstRole = Qt::UserRole + 7;
constexpr int categoryLastRole = Qt::UserRole + 8;
constexpr int shortcutKeyTextsRole = Qt::UserRole + 9;
constexpr int scopeTextRole = Qt::UserRole + 10;

kiriview::ApplicationActions::ShortcutHelpRow row(
    int actionId, const QString& actionName, const QString& actionText, const QString& shortcutText)
{
    return kiriview::ApplicationActions::ShortcutHelpRow {
        actionId,
        actionName,
        actionText,
        shortcutText,
        QStringLiteral("view"),
        QStringLiteral("View"),
        QStringLiteral("Viewer-local"),
        QStringList { shortcutText },
        true,
        true,
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
    const QList<kiriview::ApplicationActions::ShortcutHelpRow> rows {
        row(14, QStringLiteral("view_rotate_clockwise"), QStringLiteral("Rotate Clockwise"),
            QStringLiteral("Ctrl+R")),
    };
    kiriview::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, actionIdRole).toInt(), 14);
    QCOMPARE(model.data(index, actionNameRole).toString(), QStringLiteral("view_rotate_clockwise"));
    QCOMPARE(model.data(index, actionTextRole).toString(), QStringLiteral("Rotate Clockwise"));
    QCOMPARE(model.data(index, shortcutTextRole).toString(), QStringLiteral("Ctrl+R"));
    QCOMPARE(model.data(index, categoryKeyRole).toString(), QStringLiteral("view"));
    QCOMPARE(model.data(index, categoryTextRole).toString(), QStringLiteral("View"));
    QCOMPARE(model.data(index, scopeTextRole).toString(), QStringLiteral("Viewer-local"));
    QVERIFY(model.data(index, categoryFirstRole).toBool());
    QVERIFY(model.data(index, categoryLastRole).toBool());
    QCOMPARE(model.data(index, shortcutKeyTextsRole).toStringList(),
        QStringList({ QStringLiteral("Ctrl+R") }));
}

void TestShortcutHelpModel::changedRowDataUpdatesOnlyMatchingRow()
{
    QList<kiriview::ApplicationActions::ShortcutHelpRow> rows {
        row(0, QStringLiteral("file_open"), QStringLiteral("Open"), QStringLiteral("Ctrl+O")),
        row(14, QStringLiteral("view_rotate_clockwise"), QStringLiteral("Rotate Clockwise"),
            QStringLiteral("Ctrl+R")),
    };
    kiriview::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    rows[1].shortcutText = QStringLiteral("Alt+R");
    rows[1].shortcutKeyTexts = QStringList({ QStringLiteral("Alt+R") });
    model.handleRowsChanged();

    QCOMPARE(dataChangedSpy.count(), 1);
    const QModelIndex changedIndex = dataChangedSpy.at(0).at(0).toModelIndex();
    QCOMPARE(changedIndex.row(), 1);
    QCOMPARE(model.data(model.index(1, 0), shortcutTextRole).toString(), QStringLiteral("Alt+R"));
}

void TestShortcutHelpModel::changedRowIdentityResetsModel()
{
    QList<kiriview::ApplicationActions::ShortcutHelpRow> rows {
        row(0, QStringLiteral("file_open"), QStringLiteral("Open"), QStringLiteral("Ctrl+O")),
    };
    kiriview::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    rows.clear();
    model.handleRowsChanged();

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 0);
}

QTEST_MAIN(TestShortcutHelpModel)

#include "test_shortcuthelpmodel.moc"
