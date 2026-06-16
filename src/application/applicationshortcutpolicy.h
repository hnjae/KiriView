// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSHORTCUTPOLICY_H
#define KIRIVIEW_APPLICATIONSHORTCUTPOLICY_H

#include "application/applicationactionstatepolicy.h"
#include "application/applicationtypes.h"
#include "application/imageactionavailabilitypolicy.h"
#include "application/kiriviewapplicationactions.h"

#include <QKeySequence>
#include <QList>
#include <QString>
#include <QtGlobal>
#include <optional>

namespace kiriview::ApplicationActions {
struct ApplicationShortcutRoute {
    QList<ActionId> actionIds;
    ApplicationShortcutActivationScope activationScope
        = ApplicationShortcutActivationScope::ProgramWide;
    ImageShortcutScope shortcutScope = ImageShortcutScope::HelpShortcutScope;
};

struct ApplicationShortcutProjection {
    QList<QKeySequence> shortcuts;
    QList<QKeySequence> programWideShortcuts;
    QList<QKeySequence> viewerLocalShortcuts;
    QKeySequence menuShortcut;
    QString shortcutText;
    QString menuShortcutText;
};

enum class FixedShortcutDispatchKind {
    None,
    HorizontalArrow,
    SinglePageArrow,
    VerticalPan,
    VideoSeek,
};

struct FixedShortcutDispatchInput {
    bool videoMode = false;
    bool helpActionsEnabled = false;
    bool viewerShortcutsEnabled = false;
    bool readyViewerShortcutsEnabled = false;
    bool videoFileDeletionInProgress = false;
    bool activeNavigationActionsAvailable = false;
    bool twoPageViewerShortcutsEnabled = false;
    bool pannableViewerShortcutsEnabled = false;
};

struct FixedShortcutDispatchOutcome {
    FixedShortcutDispatchKind kind = FixedShortcutDispatchKind::None;
    bool previousOrUp = false;
    qint64 videoSeekDeltaMilliseconds = 0;
};

enum class GenericShortcutDispatchKind {
    None,
    TriggerAction,
    UnsupportedVideoAction,
    UnsupportedImageAction,
};

struct GenericShortcutBinding {
    ActionId actionId = ActionId::ActionCount;
    std::optional<ImageShortcutScope> shortcutScope;
    QList<QKeySequence> shortcuts;
    bool actionEnabled = false;
};

struct GenericShortcutDispatchInput {
    ApplicationActionStateInput actionState;
    QList<GenericShortcutBinding> bindings;
};

struct GenericShortcutDispatchOutcome {
    GenericShortcutDispatchKind kind = GenericShortcutDispatchKind::None;
    ActionId actionId = ActionId::ActionCount;
};

QKeySequence menuShortcut(const QList<QKeySequence> &shortcuts);
QString shortcutListText(const QList<QKeySequence> &shortcuts);
QList<QKeySequence> sanitizeProgramWideShortcuts(const QList<QKeySequence> &shortcuts);
ApplicationShortcutProjection shortcutProjection(const QList<QKeySequence> &programWideShortcuts,
    const QList<QKeySequence> &viewerLocalShortcuts = {});
const QList<ApplicationShortcutRoute> &shortcutRoutes();
std::optional<ImageShortcutScope> imageShortcutScopeFromValue(int value);
FixedShortcutDispatchOutcome fixedShortcutDispatchOutcome(
    const FixedShortcutDispatchInput &input, const QKeySequence &shortcut);
bool genericShortcutBindingEnabled(
    const ApplicationActionStateInput &actionState, const GenericShortcutBinding &binding);
GenericShortcutDispatchOutcome genericShortcutDispatchOutcome(
    const GenericShortcutDispatchInput &input, const QKeySequence &shortcut);
bool videoActionUnsupported(ActionId actionId);
bool imageActionUnsupported(ActionId actionId);
}

#endif
