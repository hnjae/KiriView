// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationshortcutpolicy.h"
#include "application/kiriviewapplicationactions.h"
#include "application/shortcutroutemodel.h"
#include "facade/kiriviewapplication.h"
#include "kiriviewstate.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KirigamiActionCollection>
#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QFileInfo>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QVariantList>
#include <cstddef>

namespace {
namespace Actions = KiriView::ApplicationActions;

using Scope = KiriView::ApplicationActions::ImageShortcutScope;
using DomainActionId = KiriView::ApplicationActions::ActionId;

constexpr const char *interfaceConfigGroup = "Interface";
constexpr const char *menuPresentationConfigKey = "menuPresentation";
constexpr const char *stateConfigFileName = "kiriviewstaterc";
constexpr int shortcutHelpActionIdRole = Qt::UserRole + 1;
constexpr int shortcutHelpActionNameRole = Qt::UserRole + 2;
constexpr int shortcutHelpActionTextRole = Qt::UserRole + 3;
constexpr int shortcutHelpShortcutTextRole = Qt::UserRole + 4;

QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QVariantList actionIdVariants(const QList<DomainActionId> &actionIds)
{
    QVariantList variants;
    variants.reserve(actionIds.size());

    for (DomainActionId actionId : actionIds) {
        variants.push_back(static_cast<int>(actionId));
    }

    return variants;
}

QString nativeText(const QKeySequence &sequence)
{
    return sequence.toString(QKeySequence::NativeText);
}

QString definitionActionName(const Actions::ActionDefinition &definition)
{
    return QString::fromLatin1(definition.name);
}

bool actionDefaultShortcutsAreManagedByKiriView(const Actions::ActionDefinition &definition)
{
    return definition.kind != Actions::RegistrationKind::Inherited;
}

KiriViewApplication::ActionId facadeActionId(DomainActionId actionId)
{
    return KiriViewApplication::facadeActionId(actionId);
}

KConfigGroup stateInterfaceGroup()
{
    return KConfigGroup(KiriViewState::self()->config(), QLatin1String(interfaceConfigGroup));
}

void resetConfig()
{
    KiriViewState *state = KiriViewState::self();
    state->config()->deleteGroup(QStringLiteral("Interface"));
    state->config()->sync();
    state->config()->reparseConfiguration();
    state->read();

    KSharedConfig::Ptr appConfig = KSharedConfig::openConfig();
    appConfig->deleteGroup(QStringLiteral("Shortcuts"));
    appConfig->sync();
    appConfig->reparseConfiguration();
}

void expectDefaultShortcuts(KiriViewApplication &application, const QString &actionName,
    const QList<QKeySequence> &shortcuts)
{
    QAction *action = application.action(actionName);
    QVERIFY2(action != nullptr, qPrintable(QStringLiteral("Missing action %1").arg(actionName)));
    QCOMPARE(KirigamiActionCollection::defaultShortcuts(action), shortcuts);
    QCOMPARE(action->shortcuts(), shortcuts);
}

QModelIndex shortcutHelpIndexForAction(QAbstractItemModel *model, const QString &actionName)
{
    if (model == nullptr) {
        return {};
    }

    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        if (model->data(index, shortcutHelpActionNameRole).toString() == actionName) {
            return index;
        }
    }

    return {};
}
}

class TestKiriViewApplication : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void actionsAreRegisteredWithDefaultShortcuts();
    void generalSettingsActionIsNotRegistered();
    void actionDefinitionTableIsCanonicalIdentitySource();
    void facadeActionIdsConvertAtApplicationBoundary();
    void actionIdsResolveActionNamesAndShortcuts();
    void shortcutRouteModelExposesApplicationPolicy();
    void videoShortcutPolicyApiExposesApplicationPolicy();
    void shortcutsApiReturnsCurrentShortcuts();
    void shortcutModifierPartitionsTextInputShortcuts();
    void shortcutAliasesDeriveFromCtrlShortcuts();
    void menuShortcutTextReturnsFirstDisplaySafeShortcut();
    void shortcutRevisionTracksShortcutChanges();
    void fixedCommandShortcutsAreNonConfigurable();
    void shortcutHelpModelListsConfigurableActions();
    void shortcutHelpModelUpdatesShortcutText();
    void shortcutHelpModelResetsWhenConfigurableRowsChange();
    void menuPresentationDefaultsToHamburgerMenu();
    void invalidStoredMenuPresentationFallsBackToHamburgerMenu();
    void menuPresentationPersists();
    void menuPresentationStateUsesGenericStateLocation();
    void showMenubarActionTogglesMenuPresentation();
};

void TestKiriViewApplication::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    resetConfig();
}

void TestKiriViewApplication::init() { resetConfig(); }

void TestKiriViewApplication::cleanup() { resetConfig(); }

void TestKiriViewApplication::actionsAreRegisteredWithDefaultShortcuts()
{
    KiriViewApplication application;

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        const QString actionName = definitionActionName(definition);
        QVERIFY2(application.action(actionName) != nullptr,
            qPrintable(QStringLiteral("Missing action %1").arg(actionName)));
        if (actionDefaultShortcutsAreManagedByKiriView(definition)) {
            expectDefaultShortcuts(
                application, actionName, Actions::defaultShortcuts(definition.defaultShortcuts));
        }
    }
}

void TestKiriViewApplication::generalSettingsActionIsNotRegistered()
{
    KiriViewApplication application;

    QCOMPARE(application.action(QStringLiteral("options_configure")), nullptr);
    QCOMPARE(application.shortcuts(QStringLiteral("options_configure")), QList<QKeySequence>());
}

void TestKiriViewApplication::actionDefinitionTableIsCanonicalIdentitySource()
{
    const auto &definitions = Actions::definitions();
    QCOMPARE(definitions.size(), Actions::actionDefinitionCount);

    for (std::size_t index = 0; index < definitions.size(); ++index) {
        const auto actionId = static_cast<DomainActionId>(index);
        const Actions::ActionDefinition *definition = Actions::definitionForId(actionId);

        QVERIFY(definition != nullptr);
        QCOMPARE(definition, &definitions.at(index));
        QCOMPARE(definition->actionId, actionId);
        QCOMPARE(Actions::actionName(actionId), definitionActionName(*definition));
    }

    QVERIFY(Actions::definitionForId(static_cast<DomainActionId>(-1)) == nullptr);
    QVERIFY(Actions::definitionForId(DomainActionId::ActionCount) == nullptr);
    QVERIFY(Actions::actionName(static_cast<DomainActionId>(-1)).isEmpty());
    QVERIFY(Actions::actionName(DomainActionId::ActionCount).isEmpty());
}

void TestKiriViewApplication::facadeActionIdsConvertAtApplicationBoundary()
{
    QCOMPARE(KiriViewApplication::domainActionId(KiriViewApplication::FileOpenAction),
        DomainActionId::FileOpenAction);
    QCOMPARE(KiriViewApplication::facadeActionId(DomainActionId::ViewRotateClockwiseAction),
        KiriViewApplication::ViewRotateClockwiseAction);
    QCOMPARE(KiriViewApplication::domainActionId(KiriViewApplication::ActionCount),
        DomainActionId::ActionCount);
    QCOMPARE(KiriViewApplication::facadeActionId(static_cast<DomainActionId>(999)),
        static_cast<KiriViewApplication::ActionId>(999));

    QCOMPARE(KiriViewApplication::domainMenuPresentation(KiriViewApplication::MenuBar),
        Actions::MenuPresentation::MenuBar);
    QCOMPARE(KiriViewApplication::facadeMenuPresentation(Actions::MenuPresentation::HamburgerMenu),
        KiriViewApplication::HamburgerMenu);
    QCOMPARE(KiriViewApplication::domainMenuPresentation(
                 static_cast<KiriViewApplication::MenuPresentation>(99)),
        Actions::MenuPresentation::HamburgerMenu);
}

void TestKiriViewApplication::actionIdsResolveActionNamesAndShortcuts()
{
    KiriViewApplication application;

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        const QString actionName = definitionActionName(definition);
        const KiriViewApplication::ActionId actionId = facadeActionId(definition.actionId);
        QCOMPARE(application.actionName(actionId), actionName);
        QCOMPARE(application.actionForId(actionId), application.action(actionName));
        QCOMPARE(application.shortcutsForId(actionId), application.shortcuts(actionName));
        QCOMPARE(application.shortcutsWithCommandModifierForId(actionId),
            application.shortcutsWithCommandModifier(actionName));
        QCOMPARE(application.shortcutsWithoutCommandModifierForId(actionId),
            application.shortcutsWithoutCommandModifier(actionName));
        QCOMPARE(
            application.shortcutAliasesForId(actionId), application.shortcutAliases(actionName));
        QCOMPARE(application.shortcutTextForId(actionId), application.shortcutText(actionName));
        QCOMPARE(application.menuShortcutForId(actionId), application.menuShortcut(actionName));
        QCOMPARE(
            application.menuShortcutTextForId(actionId), application.menuShortcutText(actionName));
    }

    const auto invalidActionId = static_cast<KiriViewApplication::ActionId>(-1);
    QVERIFY(application.actionName(invalidActionId).isEmpty());
    QCOMPARE(application.actionForId(invalidActionId), nullptr);
    QCOMPARE(application.shortcutsForId(invalidActionId), QList<QKeySequence>());
    QCOMPARE(application.shortcutAliasesForId(invalidActionId), QList<QKeySequence>());
    QVERIFY(application.shortcutTextForId(invalidActionId).isEmpty());
    QVERIFY(application.menuShortcutForId(invalidActionId).isEmpty());
    QVERIFY(application.menuShortcutTextForId(invalidActionId).isEmpty());

    const auto outOfRangeActionId = static_cast<KiriViewApplication::ActionId>(999);
    QVERIFY(application.actionName(outOfRangeActionId).isEmpty());
    QCOMPARE(application.actionForId(outOfRangeActionId), nullptr);
    QCOMPARE(application.shortcutsForId(outOfRangeActionId), QList<QKeySequence>());
    QCOMPARE(application.shortcutAliasesForId(outOfRangeActionId), QList<QKeySequence>());
    QVERIFY(application.shortcutTextForId(outOfRangeActionId).isEmpty());
    QVERIFY(application.menuShortcutForId(outOfRangeActionId).isEmpty());
    QVERIFY(application.menuShortcutTextForId(outOfRangeActionId).isEmpty());

    QVERIFY(application.actionName(KiriViewApplication::ActionCount).isEmpty());
    QCOMPARE(application.actionForId(KiriViewApplication::ActionCount), nullptr);
    QCOMPARE(application.shortcutsForId(KiriViewApplication::ActionCount), QList<QKeySequence>());
    QCOMPARE(
        application.shortcutAliasesForId(KiriViewApplication::ActionCount), QList<QKeySequence>());
    QVERIFY(application.shortcutTextForId(KiriViewApplication::ActionCount).isEmpty());
    QVERIFY(application.menuShortcutForId(KiriViewApplication::ActionCount).isEmpty());
    QVERIFY(application.menuShortcutTextForId(KiriViewApplication::ActionCount).isEmpty());
}

void TestKiriViewApplication::shortcutRouteModelExposesApplicationPolicy()
{
    KiriViewApplication application;
    QAbstractItemModel *model = application.shortcutRouteModel();
    QVERIFY(model != nullptr);

    const QList<Actions::ApplicationShortcutRoute> &routes = Actions::shortcutRoutes();
    QCOMPARE(model->rowCount(), static_cast<int>(routes.size()));

    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        const Actions::ApplicationShortcutRoute &route = routes.at(row);
        QCOMPARE(model->data(index, Actions::ShortcutRouteModel::ActionIdsRole).toList(),
            actionIdVariants(route.actionIds));
        QCOMPARE(model->data(index, Actions::ShortcutRouteModel::ShortcutFilterRole).toInt(),
            static_cast<int>(route.shortcutFilter));
        QCOMPARE(model->data(index, Actions::ShortcutRouteModel::ShortcutScopeRole).toInt(),
            static_cast<int>(route.shortcutScope));
    }
}

void TestKiriViewApplication::videoShortcutPolicyApiExposesApplicationPolicy()
{
    KiriViewApplication application;

    QVERIFY(application.videoShortcutsEnabledForScope(
        static_cast<int>(Scope::ReadyViewerShortcutScope), true, true, false, true));
    QVERIFY(!application.videoShortcutsEnabledForScope(
        static_cast<int>(Scope::ReadyViewerShortcutScope), true, false, false, true));
    QVERIFY(!application.videoShortcutsEnabledForScope(
        static_cast<int>(Scope::ImageSelectionShortcutScope), true, true, false, false));
    QVERIFY(!application.videoShortcutsEnabledForScope(999, true, true, false, true));

    QVERIFY(application.videoActionUnsupported(KiriViewApplication::ViewZoomInAction));
    QVERIFY(!application.videoActionUnsupported(KiriViewApplication::WindowFullscreenAction));

    QVERIFY(application.mediaHorizontalArrowShortcutsEnabled(false, true, false, false, false));
    QVERIFY(!application.mediaHorizontalArrowShortcutsEnabled(false, false, true, true, false));
    QVERIFY(application.mediaHorizontalArrowShortcutsEnabled(true, false, true, true, false));
    QVERIFY(!application.mediaHorizontalArrowShortcutsEnabled(true, true, true, true, true));
}

void TestKiriViewApplication::shortcutsApiReturnsCurrentShortcuts()
{
    KiriViewApplication application;

    QCOMPARE(application.shortcuts(QStringLiteral("view_zoom_in")),
        QList<QKeySequence>(
            { shortcut(QStringLiteral("Ctrl+=")), shortcut(QStringLiteral("Ctrl++")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_zoom_out")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+-")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_toggle_two_page_mode")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+S")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_toggle_right_to_left_reading")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+B")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_toggle_info_panel")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+I")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_toggle_thumbnail_panel")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+T")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("options_show_menubar")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+M")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("open_application_menu")),
        QList<QKeySequence>({ shortcut(QStringLiteral("F10")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_rotate_clockwise")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+R")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_rotate_counterclockwise")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+Shift+R")) }));
    QVERIFY(application.action(QStringLiteral("go_previous_single_page")) == nullptr);
    QCOMPARE(
        application.shortcuts(QStringLiteral("go_previous_single_page")), QList<QKeySequence>());
    QVERIFY(application.action(QStringLiteral("go_next_single_page")) == nullptr);
    QCOMPARE(application.shortcuts(QStringLiteral("go_next_single_page")), QList<QKeySequence>());
    QVERIFY(application.action(QStringLiteral("view_pan_left")) == nullptr);
    QCOMPARE(application.shortcuts(QStringLiteral("view_pan_left")), QList<QKeySequence>());
    QCOMPARE(application.shortcuts(QStringLiteral("missing_action")), QList<QKeySequence>());

    QAction *openAction = application.action(QStringLiteral("file_open"));
    QVERIFY(openAction != nullptr);
    const QList<QKeySequence> customShortcuts = {
        shortcut(QStringLiteral("Alt+O")),
        shortcut(QStringLiteral("Ctrl+Shift+O")),
    };

    openAction->setShortcuts(customShortcuts);

    QCOMPARE(application.shortcuts(QStringLiteral("file_open")), customShortcuts);

    openAction->setShortcuts({ shortcut(QStringLiteral("O")) });
    QCOMPARE(application.shortcuts(QStringLiteral("file_open")), QList<QKeySequence>());
}

void TestKiriViewApplication::shortcutModifierPartitionsTextInputShortcuts()
{
    KiriViewApplication application;

    QCOMPARE(application.shortcutsWithoutCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>());
    QCOMPARE(application.shortcutsWithCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+Q")) }));
    QCOMPARE(application.shortcutsWithoutCommandModifier(QStringLiteral("view_rotate_clockwise")),
        QList<QKeySequence>());
    QCOMPARE(application.shortcutsWithCommandModifier(QStringLiteral("view_rotate_clockwise")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+R")) }));
    QCOMPARE(
        application.shortcutsWithoutCommandModifier(QStringLiteral("view_rotate_counterclockwise")),
        QList<QKeySequence>());
    QCOMPARE(
        application.shortcutsWithCommandModifier(QStringLiteral("view_rotate_counterclockwise")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+Shift+R")) }));
    QCOMPARE(application.shortcutsWithCommandModifier(QStringLiteral("missing_action")),
        QList<QKeySequence>());
    QCOMPARE(application.shortcutsWithoutCommandModifier(QStringLiteral("missing_action")),
        QList<QKeySequence>());

    QAction *quitAction = application.action(QStringLiteral("file_quit"));
    QVERIFY(quitAction != nullptr);
    quitAction->setShortcuts({ shortcut(QStringLiteral("Alt+Q")),
        shortcut(QStringLiteral("Shift+Q")), shortcut(QStringLiteral("Meta+Q")),
        shortcut(QStringLiteral("Ctrl+Shift+Q")), shortcut(QStringLiteral("Q")) });

    QCOMPARE(quitAction->shortcuts(),
        QList<QKeySequence>(
            { shortcut(QStringLiteral("Alt+Q")), shortcut(QStringLiteral("Shift+Q")),
                shortcut(QStringLiteral("Meta+Q")), shortcut(QStringLiteral("Ctrl+Shift+Q")) }));
    QCOMPARE(application.shortcutsWithCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Alt+Q")), shortcut(QStringLiteral("Meta+Q")),
            shortcut(QStringLiteral("Ctrl+Shift+Q")) }));
    QCOMPARE(application.shortcutsWithoutCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Shift+Q")) }));
}

void TestKiriViewApplication::shortcutAliasesDeriveFromCtrlShortcuts()
{
    KiriViewApplication application;

    QCOMPARE(application.shortcutAliases(QStringLiteral("view_rotate_clockwise")),
        QList<QKeySequence>({ shortcut(QStringLiteral("R")) }));
    QCOMPARE(
        application.shortcutAliasesForId(KiriViewApplication::ViewRotateCounterclockwiseAction),
        QList<QKeySequence>({ shortcut(QStringLiteral("Shift+R")) }));
    QCOMPARE(application.shortcutAliases(QStringLiteral("view_zoom_in")),
        QList<QKeySequence>({ shortcut(QStringLiteral("=")), shortcut(QStringLiteral("+")) }));
    QCOMPARE(application.shortcutAliases(QStringLiteral("view_toggle_info_panel")),
        QList<QKeySequence>({ shortcut(QStringLiteral("I")) }));
    QCOMPARE(application.shortcutAliases(QStringLiteral("view_toggle_thumbnail_panel")),
        QList<QKeySequence>({ shortcut(QStringLiteral("T")) }));
    QCOMPARE(application.shortcutAliases(QStringLiteral("movetotrash")), QList<QKeySequence>());
    QCOMPARE(
        application.shortcutAliases(QStringLiteral("options_show_menubar")), QList<QKeySequence>());
    QCOMPARE(application.shortcutAliases(QStringLiteral("open_application_menu")),
        QList<QKeySequence>());
    QCOMPARE(application.shortcutAliases(QStringLiteral("missing_action")), QList<QKeySequence>());
    QCOMPARE(application.shortcutAliasesForId(static_cast<KiriViewApplication::ActionId>(999)),
        QList<QKeySequence>());

    QAction *rotateAction = application.action(QStringLiteral("view_rotate_clockwise"));
    QVERIFY(rotateAction != nullptr);
    rotateAction->setShortcuts({ shortcut(QStringLiteral("Ctrl+L")) });

    QCOMPARE(application.shortcutAliasesForId(KiriViewApplication::ViewRotateClockwiseAction),
        QList<QKeySequence>({ shortcut(QStringLiteral("L")) }));
    QCOMPARE(application.menuShortcutTextForId(KiriViewApplication::ViewRotateClockwiseAction),
        nativeText(shortcut(QStringLiteral("Ctrl+L"))));
}

void TestKiriViewApplication::menuShortcutTextReturnsFirstDisplaySafeShortcut()
{
    KiriViewApplication application;

    QCOMPARE(application.menuShortcutText(QStringLiteral("view_rotate_clockwise")),
        nativeText(shortcut(QStringLiteral("Ctrl+R"))));
    QCOMPARE(application.menuShortcut(QStringLiteral("view_rotate_clockwise")),
        shortcut(QStringLiteral("Ctrl+R")));
    QCOMPARE(application.menuShortcutTextForId(KiriViewApplication::ViewRotateClockwiseAction),
        nativeText(shortcut(QStringLiteral("Ctrl+R"))));
    QCOMPARE(application.menuShortcutForId(KiriViewApplication::ViewRotateClockwiseAction),
        shortcut(QStringLiteral("Ctrl+R")));
    QCOMPARE(application.shortcutText(QStringLiteral("view_rotate_clockwise")),
        nativeText(shortcut(QStringLiteral("Ctrl+R"))));

    QVERIFY(application.menuShortcutText(QStringLiteral("movetotrash")).isEmpty());
    QVERIFY(application.menuShortcutForId(KiriViewApplication::FileMoveToTrashAction).isEmpty());
    QVERIFY(application.menuShortcutText(QStringLiteral("deletefile")).isEmpty());
    QVERIFY(application.menuShortcutText(QStringLiteral("view_pan_left")).isEmpty());
    QVERIFY(application.menuShortcutText(QStringLiteral("go_previous_image")).isEmpty());

    QAction *openAction = application.action(QStringLiteral("file_open"));
    QVERIFY(openAction != nullptr);
    openAction->setShortcuts({ QKeySequence(), shortcut(QStringLiteral("Delete")),
        shortcut(QStringLiteral("Alt+O")), shortcut(QStringLiteral("Ctrl+Shift+O")) });

    QCOMPARE(application.menuShortcutText(QStringLiteral("file_open")),
        nativeText(shortcut(QStringLiteral("Alt+O"))));
    QCOMPARE(
        application.menuShortcut(QStringLiteral("file_open")), shortcut(QStringLiteral("Alt+O")));
    QCOMPARE(application.menuShortcutTextForId(KiriViewApplication::FileOpenAction),
        nativeText(shortcut(QStringLiteral("Alt+O"))));
    QCOMPARE(application.shortcutText(QStringLiteral("file_open")),
        QStringLiteral("%1 / %2 / %3")
            .arg(nativeText(shortcut(QStringLiteral("Delete"))),
                nativeText(shortcut(QStringLiteral("Alt+O"))),
                nativeText(shortcut(QStringLiteral("Ctrl+Shift+O")))));

    QCOMPARE(application.menuShortcutText(QStringLiteral("options_show_menubar")),
        nativeText(shortcut(QStringLiteral("Ctrl+M"))));
    QCOMPARE(application.menuShortcutForId(KiriViewApplication::OptionsShowMenubarAction),
        shortcut(QStringLiteral("Ctrl+M")));
    QCOMPARE(application.menuShortcutTextForId(KiriViewApplication::OptionsShowMenubarAction),
        nativeText(shortcut(QStringLiteral("Ctrl+M"))));
    QCOMPARE(application.menuShortcutText(QStringLiteral("open_application_menu")),
        nativeText(shortcut(QStringLiteral("F10"))));
    QVERIFY(application.menuShortcutText(QStringLiteral("missing_action")).isEmpty());
    QVERIFY(
        application.menuShortcutForId(static_cast<KiriViewApplication::ActionId>(999)).isEmpty());
    QVERIFY(application.menuShortcutTextForId(static_cast<KiriViewApplication::ActionId>(999))
            .isEmpty());
}

void TestKiriViewApplication::shortcutRevisionTracksShortcutChanges()
{
    KiriViewApplication application;
    QSignalSpy revisionSpy(&application, &KiriViewApplication::shortcutRevisionChanged);
    QAction *rotateAction = application.action(QStringLiteral("view_rotate_clockwise"));
    QVERIFY(rotateAction != nullptr);

    const int initialRevision = application.shortcutRevision();

    rotateAction->setShortcuts({ shortcut(QStringLiteral("Alt+R")) });

    QTRY_COMPARE(revisionSpy.count(), 1);
    QCOMPARE(application.shortcutRevision(), initialRevision + 1);
    QCOMPARE(application.shortcuts(QStringLiteral("view_rotate_clockwise")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Alt+R")) }));

    rotateAction->setShortcuts({ shortcut(QStringLiteral("R")) });

    QTRY_COMPARE(revisionSpy.count(), 2);
    QCOMPARE(application.shortcutRevision(), initialRevision + 2);
    QCOMPARE(rotateAction->shortcuts(), QList<QKeySequence>());
    QCOMPARE(application.shortcuts(QStringLiteral("view_rotate_clockwise")), QList<QKeySequence>());
}

void TestKiriViewApplication::fixedCommandShortcutsAreNonConfigurable()
{
    KiriViewApplication application;

    QAction *showMenubarAction = application.action(QStringLiteral("options_show_menubar"));
    QVERIFY(showMenubarAction != nullptr);
    QCOMPARE(KirigamiActionCollection::defaultShortcuts(showMenubarAction),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+M")) }));
    QCOMPARE(showMenubarAction->shortcuts(),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+M")) }));
    QVERIFY(!KirigamiActionCollection::isShortcutsConfigurable(showMenubarAction));

    QAction *openApplicationMenuAction
        = application.action(QStringLiteral("open_application_menu"));
    QVERIFY(openApplicationMenuAction != nullptr);
    QCOMPARE(KirigamiActionCollection::defaultShortcuts(openApplicationMenuAction),
        QList<QKeySequence>({ shortcut(QStringLiteral("F10")) }));
    QCOMPARE(openApplicationMenuAction->shortcuts(),
        QList<QKeySequence>({ shortcut(QStringLiteral("F10")) }));
    QVERIFY(!KirigamiActionCollection::isShortcutsConfigurable(openApplicationMenuAction));
}

void TestKiriViewApplication::shortcutHelpModelListsConfigurableActions()
{
    KiriViewApplication application;

    QAbstractItemModel *model = application.shortcutHelpModel();
    QVERIFY(model != nullptr);

    const QModelIndex moveToTrashIndex
        = shortcutHelpIndexForAction(model, QStringLiteral("movetotrash"));
    QVERIFY(moveToTrashIndex.isValid());
    QCOMPARE(model->data(moveToTrashIndex, shortcutHelpActionIdRole).toInt(),
        static_cast<int>(KiriViewApplication::FileMoveToTrashAction));
    QCOMPARE(model->data(moveToTrashIndex, shortcutHelpActionTextRole).toString(),
        QStringLiteral("Move to Trash"));
    QCOMPARE(model->data(moveToTrashIndex, shortcutHelpShortcutTextRole).toString(),
        nativeText(shortcut(QStringLiteral("Delete"))));

    const QModelIndex deleteIndex = shortcutHelpIndexForAction(model, QStringLiteral("deletefile"));
    QVERIFY(deleteIndex.isValid());
    QCOMPARE(model->data(deleteIndex, shortcutHelpActionTextRole).toString(),
        QStringLiteral("Delete Permanently"));

    QVERIFY(
        !shortcutHelpIndexForAction(model, QStringLiteral("go_previous_single_page")).isValid());
    QVERIFY(!shortcutHelpIndexForAction(model, QStringLiteral("go_next_single_page")).isValid());
    QVERIFY(!shortcutHelpIndexForAction(model, QStringLiteral("options_show_menubar")).isValid());
    QVERIFY(!shortcutHelpIndexForAction(model, QStringLiteral("open_application_menu")).isValid());
}

void TestKiriViewApplication::shortcutHelpModelUpdatesShortcutText()
{
    KiriViewApplication application;

    QAbstractItemModel *model = application.shortcutHelpModel();
    QVERIFY(model != nullptr);
    const QModelIndex rotateIndex
        = shortcutHelpIndexForAction(model, QStringLiteral("view_rotate_clockwise"));
    QVERIFY(rotateIndex.isValid());
    QCOMPARE(model->data(rotateIndex, shortcutHelpShortcutTextRole).toString(),
        nativeText(shortcut(QStringLiteral("Ctrl+R"))));

    QSignalSpy dataChangedSpy(model, &QAbstractItemModel::dataChanged);
    QAction *rotateAction = application.action(QStringLiteral("view_rotate_clockwise"));
    QVERIFY(rotateAction != nullptr);

    rotateAction->setShortcuts({ shortcut(QStringLiteral("Alt+R")) });

    QTRY_VERIFY(!dataChangedSpy.isEmpty());
    QCOMPARE(model->data(rotateIndex, shortcutHelpShortcutTextRole).toString(),
        nativeText(shortcut(QStringLiteral("Alt+R"))));
}

void TestKiriViewApplication::shortcutHelpModelResetsWhenConfigurableRowsChange()
{
    KiriViewApplication application;

    QAbstractItemModel *model = application.shortcutHelpModel();
    QVERIFY(model != nullptr);
    const QModelIndex rotateIndex
        = shortcutHelpIndexForAction(model, QStringLiteral("view_rotate_clockwise"));
    QVERIFY(rotateIndex.isValid());

    QSignalSpy resetSpy(model, &QAbstractItemModel::modelReset);
    QAction *rotateAction = application.action(QStringLiteral("view_rotate_clockwise"));
    QVERIFY(rotateAction != nullptr);

    KirigamiActionCollection::setShortcutsConfigurable(rotateAction, false);
    rotateAction->changed();

    QTRY_COMPARE(resetSpy.count(), 1);
    QVERIFY(!shortcutHelpIndexForAction(model, QStringLiteral("view_rotate_clockwise")).isValid());
}

void TestKiriViewApplication::menuPresentationDefaultsToHamburgerMenu()
{
    KiriViewApplication application;

    QCOMPARE(KiriViewState::defaultMenuPresentationValue(),
        static_cast<int>(KiriViewState::EnumMenuPresentation::HamburgerMenu));
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);
    QCOMPARE(KiriViewState::menuPresentation(),
        static_cast<int>(KiriViewState::EnumMenuPresentation::HamburgerMenu));
    QVERIFY(!stateInterfaceGroup().hasKey(QLatin1String(menuPresentationConfigKey)));
}

void TestKiriViewApplication::invalidStoredMenuPresentationFallsBackToHamburgerMenu()
{
    KiriViewState::setMenuPresentation(99);

    KiriViewApplication application;

    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);
}

void TestKiriViewApplication::menuPresentationPersists()
{
    {
        KiriViewApplication application;
        QSignalSpy changedSpy(&application, &KiriViewApplication::menuPresentationChanged);

        application.setMenuPresentation(KiriViewApplication::MenuBar);

        QCOMPARE(changedSpy.count(), 1);
        QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
        QCOMPARE(KiriViewState::menuPresentation(),
            static_cast<int>(KiriViewState::EnumMenuPresentation::MenuBar));
        QCOMPARE(
            stateInterfaceGroup().readEntry(QLatin1String(menuPresentationConfigKey), QString()),
            QStringLiteral("MenuBar"));

        KiriViewState::setMenuPresentation(KiriViewState::EnumMenuPresentation::HamburgerMenu);
        QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
    }

    KiriViewState::self()->read();
    KiriViewApplication reloadedApplication;
    QCOMPARE(reloadedApplication.menuPresentation(), KiriViewApplication::MenuBar);
}

void TestKiriViewApplication::menuPresentationStateUsesGenericStateLocation()
{
    KiriViewState::setMenuPresentation(KiriViewState::EnumMenuPresentation::MenuBar);
    KiriViewState::self()->save();

    const QString statePath = QStandardPaths::locate(
        QStandardPaths::GenericStateLocation, QLatin1String(stateConfigFileName));
    QVERIFY(!statePath.isEmpty());
    QCOMPARE(QFileInfo(statePath).absolutePath(),
        QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation));
}

void TestKiriViewApplication::showMenubarActionTogglesMenuPresentation()
{
    KiriViewApplication application;

    QAction *showMenubarAction = application.action(QStringLiteral("options_show_menubar"));
    QVERIFY(showMenubarAction != nullptr);
    QVERIFY(showMenubarAction->isCheckable());
    QVERIFY(!showMenubarAction->isChecked());
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);

    showMenubarAction->trigger();
    QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
    QVERIFY(showMenubarAction->isChecked());

    showMenubarAction->trigger();
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);
    QVERIFY(!showMenubarAction->isChecked());
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    TestKiriViewApplication test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_kiriviewapplication.moc"
