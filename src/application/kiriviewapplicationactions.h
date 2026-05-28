// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIVIEWAPPLICATIONACTIONS_H
#define KIRIVIEW_KIRIVIEWAPPLICATIONACTIONS_H

#include "application/applicationtypes.h"

#include <KLazyLocalizedString>
#include <KStandardActions>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <array>
#include <cstddef>

namespace KiriView::ApplicationActions {
enum class RegistrationKind {
    Existing,
    Inherited,
    Registered,
    ShowMenubar,
    Standard,
};

enum class ShortcutSpecKind {
    None,
    PortableText,
    StandardKey,
};

enum class ShortcutConfigurability {
    Configurable,
    NonConfigurable,
};

enum class ShortcutAliasPolicy {
    DeriveViewerAlias,
    NoAlias,
};

enum class ShortcutHelpCategory {
    File,
    Navigation,
    View,
    Panels,
    Window,
    Settings,
    Help,
};

inline constexpr std::size_t maxPortableShortcutCount = 4;

struct DefaultShortcutSpec {
    ShortcutSpecKind kind = ShortcutSpecKind::None;
    QKeySequence::StandardKey standardKey = QKeySequence::UnknownKey;
    std::array<const char *, maxPortableShortcutCount> portableText {};
    std::size_t portableTextCount = 0;
};

struct DefaultShortcutRouteSpec {
    std::array<ShortcutRouteSpec, maxShortcutRouteSpecCount> specs {};
    std::size_t count = 0;
};

struct ActionDefinition {
    ActionId actionId;
    const char *name;
    RegistrationKind kind;
    KStandardActions::StandardAction actionType;
    KLazyLocalizedString text;
    const char *iconName;
    DefaultShortcutSpec defaultShortcuts;
    DefaultShortcutRouteSpec shortcutRoutes;
    ShortcutHelpCategory shortcutHelpCategory = ShortcutHelpCategory::Help;
    ShortcutConfigurability shortcutConfigurability = ShortcutConfigurability::Configurable;
    ShortcutAliasPolicy shortcutAliasPolicy = ShortcutAliasPolicy::DeriveViewerAlias;
};

const std::array<ActionDefinition, actionDefinitionCount> &definitions();
const ActionDefinition *definitionForId(ActionId actionId);
const ActionDefinition *definitionForName(const QString &actionName);
QString actionName(ActionId actionId);
QString latin1String(const char *text);
QString localizedString(const KLazyLocalizedString &text);
QString shortcutHelpCategoryKey(ShortcutHelpCategory category);
QString shortcutHelpCategoryText(ShortcutHelpCategory category);
int shortcutHelpCategoryOrder(ShortcutHelpCategory category);
QList<QKeySequence> defaultShortcuts(const DefaultShortcutSpec &spec);
}

#endif
