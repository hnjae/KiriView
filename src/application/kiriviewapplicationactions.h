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

inline constexpr std::size_t maxPortableShortcutCount = 4;

struct DefaultShortcutSpec {
    ShortcutSpecKind kind = ShortcutSpecKind::None;
    QKeySequence::StandardKey standardKey = QKeySequence::UnknownKey;
    std::array<const char *, maxPortableShortcutCount> portableText {};
    std::size_t portableTextCount = 0;
};

struct ActionDefinition {
    ActionId actionId;
    const char *name;
    RegistrationKind kind;
    KStandardActions::StandardAction actionType;
    KLazyLocalizedString text;
    const char *iconName;
    DefaultShortcutSpec defaultShortcuts;
};

const std::array<ActionDefinition, actionDefinitionCount> &definitions();
const ActionDefinition *definitionForId(ActionId actionId);
QString actionName(ActionId actionId);
QString latin1String(const char *text);
QString localizedString(const KLazyLocalizedString &text);
QList<QKeySequence> defaultShortcuts(const DefaultShortcutSpec &spec);
}

#endif
