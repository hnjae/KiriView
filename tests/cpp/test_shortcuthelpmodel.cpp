// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "shortcuthelpmodel.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QKeySequence>
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

QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QString nativeText(const QKeySequence &sequence)
{
    return sequence.toString(QKeySequence::NativeText);
}
}

class TestShortcutHelpModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rowsComeFromRuntimeProvider();
    void changedActionUpdatesOnlyMatchingRow();
    void changedRowProjectionResetsModel();
};

void TestShortcutHelpModel::rowsComeFromRuntimeProvider()
{
    QAction action;
    action.setText(QStringLiteral("&Rotate Clockwise"));
    action.setShortcuts({ shortcut(QStringLiteral("Ctrl+R")) });

    const QList<KiriView::ApplicationActions::ShortcutHelpRow> rows {
        { &action, 14, QStringLiteral("view_rotate_clockwise") },
    };
    KiriView::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, actionIdRole).toInt(), 14);
    QCOMPARE(model.data(index, actionNameRole).toString(), QStringLiteral("view_rotate_clockwise"));
    QCOMPARE(model.data(index, actionTextRole).toString(), QStringLiteral("Rotate Clockwise"));
    QCOMPARE(model.data(index, shortcutTextRole).toString(),
        nativeText(shortcut(QStringLiteral("Ctrl+R"))));
}

void TestShortcutHelpModel::changedActionUpdatesOnlyMatchingRow()
{
    QAction first;
    first.setText(QStringLiteral("&Open"));
    first.setShortcuts({ shortcut(QStringLiteral("Ctrl+O")) });
    QAction second;
    second.setText(QStringLiteral("&Rotate Clockwise"));
    second.setShortcuts({ shortcut(QStringLiteral("Ctrl+R")) });

    const QList<KiriView::ApplicationActions::ShortcutHelpRow> rows {
        { &first, 0, QStringLiteral("file_open") },
        { &second, 14, QStringLiteral("view_rotate_clockwise") },
    };
    KiriView::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    second.setShortcuts({ shortcut(QStringLiteral("Alt+R")) });
    model.handleActionChanged(&second);

    QCOMPARE(dataChangedSpy.count(), 1);
    const QModelIndex changedIndex = dataChangedSpy.at(0).at(0).toModelIndex();
    QCOMPARE(changedIndex.row(), 1);
    QCOMPARE(model.data(model.index(1, 0), shortcutTextRole).toString(),
        nativeText(shortcut(QStringLiteral("Alt+R"))));
}

void TestShortcutHelpModel::changedRowProjectionResetsModel()
{
    QAction action;
    action.setText(QStringLiteral("&Open"));
    action.setShortcuts({ shortcut(QStringLiteral("Ctrl+O")) });

    QList<KiriView::ApplicationActions::ShortcutHelpRow> rows {
        { &action, 0, QStringLiteral("file_open") },
    };
    KiriView::ApplicationActions::ShortcutHelpModel model([&rows]() { return rows; });
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    rows.clear();
    model.handleActionChanged(&action);

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 0);
}

QTEST_MAIN(TestShortcutHelpModel)

#include "test_shortcuthelpmodel.moc"
