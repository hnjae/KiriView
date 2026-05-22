// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationshortcutpolicy.h"

#include <QObject>
#include <QTest>
#include <QVariantMap>

namespace {
using ActionId = KiriViewApplication::ActionId;
using Filter = KiriView::ApplicationActions::ApplicationShortcutFilter;
using Scope = KiriView::ApplicationActions::ImageShortcutScope;

QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QString nativeText(const QKeySequence &sequence)
{
    return sequence.toString(QKeySequence::NativeText);
}

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
    void shortcutRoutesOwnApplicationShortcutScopes();
    void shortcutRouteVariantsExposeQmlShape();
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

void TestApplicationShortcutPolicy::shortcutRoutesOwnApplicationShortcutScopes()
{
    const QList<KiriView::ApplicationActions::ApplicationShortcutRoute> &routes
        = KiriView::ApplicationActions::shortcutRoutes();

    QCOMPARE(routes.size(), 34);

    QCOMPARE(
        actionIdVariants(routes.at(0).actionIds), actionIdVariants({ ActionId::FileOpenAction }));
    QCOMPARE(static_cast<int>(routes.at(0).shortcutFilter),
        static_cast<int>(Filter::WithCommandModifier));
    QCOMPARE(
        static_cast<int>(routes.at(0).shortcutScope), static_cast<int>(Scope::HelpShortcutScope));

    QCOMPARE(
        actionIdVariants(routes.at(1).actionIds), actionIdVariants({ ActionId::FileOpenAction }));
    QCOMPARE(static_cast<int>(routes.at(1).shortcutFilter),
        static_cast<int>(Filter::WithoutCommandModifier));
    QCOMPARE(
        static_cast<int>(routes.at(1).shortcutScope), static_cast<int>(Scope::ViewerShortcutScope));

    QCOMPARE(actionIdVariants(routes.at(5).actionIds),
        actionIdVariants({ ActionId::FileMoveToTrashAction, ActionId::FileDeleteAction }));
    QCOMPARE(static_cast<int>(routes.at(5).shortcutFilter), static_cast<int>(Filter::AllShortcuts));
    QCOMPARE(static_cast<int>(routes.at(5).shortcutScope),
        static_cast<int>(Scope::ReadyViewerShortcutScope));

    QCOMPARE(actionIdVariants(routes.at(6).actionIds),
        actionIdVariants({ ActionId::ViewZoomInAction, ActionId::ViewZoomOutAction,
            ActionId::ViewFitAction, ActionId::ViewFitHeightAction, ActionId::ViewFitWidthAction,
            ActionId::ViewActualSizeAction, ActionId::ViewToggleTwoPageModeAction }));
    QCOMPARE(static_cast<int>(routes.at(6).shortcutFilter),
        static_cast<int>(Filter::WithCommandModifier));
    QCOMPARE(
        static_cast<int>(routes.at(6).shortcutScope), static_cast<int>(Scope::ReadyShortcutScope));

    QCOMPARE(actionIdVariants(routes.at(12).actionIds),
        actionIdVariants({ ActionId::ViewToggleRightToLeftReadingAction }));
    QCOMPARE(static_cast<int>(routes.at(12).shortcutFilter),
        static_cast<int>(Filter::WithCommandModifier));
    QCOMPARE(static_cast<int>(routes.at(12).shortcutScope),
        static_cast<int>(Scope::RightToLeftReadingShortcutScope));

    QCOMPARE(actionIdVariants(routes.at(21).actionIds),
        actionIdVariants({ ActionId::GoPreviousImageAction, ActionId::GoNextImageAction }));
    QCOMPARE(static_cast<int>(routes.at(21).shortcutFilter),
        static_cast<int>(Filter::WithCommandModifier));
    QCOMPARE(static_cast<int>(routes.at(21).shortcutScope),
        static_cast<int>(Scope::ImageSelectionShortcutScope));

    QCOMPARE(actionIdVariants(routes.at(33).actionIds),
        actionIdVariants(
            { ActionId::OptionsConfigureKeybindingAction, ActionId::OptionsShowMenubarAction }));
    QCOMPARE(
        static_cast<int>(routes.at(33).shortcutFilter), static_cast<int>(Filter::AllShortcuts));
    QCOMPARE(
        static_cast<int>(routes.at(33).shortcutScope), static_cast<int>(Scope::HelpShortcutScope));
}

void TestApplicationShortcutPolicy::shortcutRouteVariantsExposeQmlShape()
{
    const QList<KiriView::ApplicationActions::ApplicationShortcutRoute> &routes
        = KiriView::ApplicationActions::shortcutRoutes();
    const QVariantList variants = KiriView::ApplicationActions::shortcutRouteVariants();

    QCOMPARE(variants.size(), routes.size());

    for (qsizetype index = 0; index < variants.size(); ++index) {
        const QVariantMap route = variants.at(index).toMap();
        QCOMPARE(route.value(QStringLiteral("actionIds")).toList(),
            actionIdVariants(routes.at(index).actionIds));
        QCOMPARE(route.value(QStringLiteral("shortcutFilter")).toInt(),
            static_cast<int>(routes.at(index).shortcutFilter));
        QCOMPARE(route.value(QStringLiteral("shortcutScope")).toInt(),
            static_cast<int>(routes.at(index).shortcutScope));
    }
}

QTEST_GUILESS_MAIN(TestApplicationShortcutPolicy)

#include "test_applicationshortcutpolicy.moc"
