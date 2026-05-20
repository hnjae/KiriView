// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationshortcutpolicy.h"

#include <QObject>
#include <QTest>

namespace {
QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QString nativeText(const QKeySequence &sequence)
{
    return sequence.toString(QKeySequence::NativeText);
}
}

class TestApplicationShortcutPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void commandModifierFilterPartitionsShortcuts();
    void menuShortcutSkipsViewerOnlyShortcuts();
    void shortcutAliasesDeriveViewerShortcutsFromCtrlShortcuts();
    void shortcutListTextJoinsAssignedShortcuts();
    void shortcutProjectionDerivesPublicViewsFromOneShortcutList();
    void sanitizeShortcutsRemovesUnmodifiedTextInputShortcuts();
};

void TestApplicationShortcutPolicy::commandModifierFilterPartitionsShortcuts()
{
    const QList<QKeySequence> shortcuts {
        QKeySequence(),
        shortcut(QStringLiteral("Ctrl+Q")),
        shortcut(QStringLiteral("Alt+Q")),
        shortcut(QStringLiteral("Meta+Q")),
        shortcut(QStringLiteral("Shift+Q")),
        shortcut(QStringLiteral("Q")),
    };

    QCOMPARE(KiriView::ApplicationActions::filterShortcutsByCommandModifier(shortcuts, true),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+Q")), shortcut(QStringLiteral("Alt+Q")),
            shortcut(QStringLiteral("Meta+Q")) }));
    QCOMPARE(KiriView::ApplicationActions::filterShortcutsByCommandModifier(shortcuts, false),
        QList<QKeySequence>(
            { shortcut(QStringLiteral("Shift+Q")), shortcut(QStringLiteral("Q")) }));
}

void TestApplicationShortcutPolicy::menuShortcutSkipsViewerOnlyShortcuts()
{
    const QList<QKeySequence> shortcuts {
        QKeySequence(),
        shortcut(QStringLiteral("Delete")),
        shortcut(QStringLiteral("Q")),
        shortcut(QStringLiteral("Alt+O")),
        shortcut(QStringLiteral("Ctrl+Shift+O")),
    };

    QCOMPARE(
        KiriView::ApplicationActions::menuShortcut(shortcuts), shortcut(QStringLiteral("Alt+O")));
    QVERIFY(
        KiriView::ApplicationActions::menuShortcut({ shortcut(QStringLiteral("Home")) }).isEmpty());
    QVERIFY(KiriView::ApplicationActions::menuShortcut({ shortcut(QStringLiteral("Ctrl+X, Q")) })
            .isEmpty());
}

void TestApplicationShortcutPolicy::shortcutAliasesDeriveViewerShortcutsFromCtrlShortcuts()
{
    const QList<QKeySequence> shortcuts {
        shortcut(QStringLiteral("Ctrl+R")),
        shortcut(QStringLiteral("Ctrl+Shift+R")),
        shortcut(QStringLiteral("Ctrl+R")),
        shortcut(QStringLiteral("Alt+R")),
        shortcut(QStringLiteral("Ctrl+Delete")),
        shortcut(QStringLiteral("Ctrl+X, Ctrl+Y")),
    };

    QCOMPARE(KiriView::ApplicationActions::shortcutAliases(shortcuts),
        QList<QKeySequence>(
            { shortcut(QStringLiteral("R")), shortcut(QStringLiteral("Shift+R")) }));
}

void TestApplicationShortcutPolicy::shortcutListTextJoinsAssignedShortcuts()
{
    QCOMPARE(KiriView::ApplicationActions::shortcutListText({ QKeySequence() }), QString());
    QCOMPARE(KiriView::ApplicationActions::shortcutListText({ QKeySequence(),
                 shortcut(QStringLiteral("Alt+O")), shortcut(QStringLiteral("Ctrl+Shift+O")) }),
        QStringLiteral("%1 / %2").arg(nativeText(shortcut(QStringLiteral("Alt+O"))),
            nativeText(shortcut(QStringLiteral("Ctrl+Shift+O")))));
}

void TestApplicationShortcutPolicy::shortcutProjectionDerivesPublicViewsFromOneShortcutList()
{
    const QList<QKeySequence> shortcuts {
        QKeySequence(),
        shortcut(QStringLiteral("Ctrl+R")),
        shortcut(QStringLiteral("Shift+R")),
        shortcut(QStringLiteral("Home")),
        shortcut(QStringLiteral("Alt+O")),
    };

    const KiriView::ApplicationActions::ApplicationShortcutProjection projection
        = KiriView::ApplicationActions::shortcutProjection(shortcuts);

    QCOMPARE(projection.shortcuts, shortcuts);
    QCOMPARE(projection.shortcutsWithCommandModifier,
        QList<QKeySequence>(
            { shortcut(QStringLiteral("Ctrl+R")), shortcut(QStringLiteral("Alt+O")) }));
    QCOMPARE(projection.shortcutsWithoutCommandModifier,
        QList<QKeySequence>(
            { shortcut(QStringLiteral("Shift+R")), shortcut(QStringLiteral("Home")) }));
    QCOMPARE(projection.shortcutAliases, QList<QKeySequence>({ shortcut(QStringLiteral("R")) }));
    QCOMPARE(projection.menuShortcut, shortcut(QStringLiteral("Ctrl+R")));
    QCOMPARE(projection.shortcutText,
        QStringLiteral("%1 / %2 / %3 / %4")
            .arg(nativeText(shortcut(QStringLiteral("Ctrl+R"))),
                nativeText(shortcut(QStringLiteral("Shift+R"))),
                nativeText(shortcut(QStringLiteral("Home"))),
                nativeText(shortcut(QStringLiteral("Alt+O")))));
    QCOMPARE(projection.menuShortcutText, nativeText(shortcut(QStringLiteral("Ctrl+R"))));
}

void TestApplicationShortcutPolicy::sanitizeShortcutsRemovesUnmodifiedTextInputShortcuts()
{
    const QList<QKeySequence> shortcuts {
        shortcut(QStringLiteral("Q")),
        shortcut(QStringLiteral("Shift+Q")),
        shortcut(QStringLiteral("Ctrl+Q")),
        shortcut(QStringLiteral("Delete")),
    };

    QCOMPARE(KiriView::ApplicationActions::sanitizeShortcuts(shortcuts),
        QList<QKeySequence>({ shortcut(QStringLiteral("Shift+Q")),
            shortcut(QStringLiteral("Ctrl+Q")), shortcut(QStringLiteral("Delete")) }));
}

QTEST_GUILESS_MAIN(TestApplicationShortcutPolicy)

#include "test_applicationshortcutpolicy.moc"
