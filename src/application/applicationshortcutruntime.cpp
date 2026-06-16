// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutruntime.h"

#include "imageactionavailabilitypolicy.h"
#include "shortcuthelpmodel.h"
#include "shortcutroutemodel.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KirigamiActionCollection>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <algorithm>
#include <optional>
#include <utility>

namespace {
using ActionId = kiriview::ApplicationActions::ActionId;
constexpr const char *viewerLocalShortcutsGroup = "ViewerLocalShortcuts";

kiriview::ApplicationActions::ShortcutHelpCategory shortcutHelpCategory(ActionId actionId)
{
    const kiriview::ApplicationActions::ActionDefinition *definition
        = kiriview::ApplicationActions::definitionForId(actionId);
    if (definition != nullptr) {
        return definition->shortcutHelpCategory;
    }

    return kiriview::ApplicationActions::ShortcutHelpCategory::Help;
}

int shortcutHelpCategoryOrderForAction(ActionId actionId)
{
    return kiriview::ApplicationActions::shortcutHelpCategoryOrder(shortcutHelpCategory(actionId));
}

std::optional<std::size_t> actionIndex(ActionId actionId)
{
    const int index = static_cast<int>(actionId);
    if (index < 0
        || index >= static_cast<int>(kiriview::ApplicationActions::actionDefinitionCount)) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(index);
}

QStringList portableShortcutTexts(const QList<QKeySequence> &shortcuts)
{
    QStringList texts;
    texts.reserve(shortcuts.size());
    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcut.isEmpty()) {
            texts.push_back(shortcut.toString(QKeySequence::PortableText));
        }
    }

    return texts;
}

QList<QKeySequence> shortcutsFromPortableTexts(const QStringList &texts)
{
    QList<QKeySequence> shortcuts;
    shortcuts.reserve(texts.size());
    for (const QString &text : texts) {
        const QKeySequence shortcut = QKeySequence::fromString(text, QKeySequence::PortableText);
        if (!shortcut.isEmpty()) {
            shortcuts.push_back(shortcut);
        }
    }

    return shortcuts;
}

QWindow *shortcutWindow(QObject *host)
{
    auto *window = qobject_cast<QWindow *>(host);
    if (window != nullptr) {
        return window;
    }

    auto *quickItem = qobject_cast<QQuickItem *>(host);
    if (quickItem != nullptr) {
        return quickItem->window();
    }

    return nullptr;
}

class ShortcutEventFilter final : public QObject
{
public:
    using Handler = std::function<bool(const QKeySequence &)>;

    explicit ShortcutEventFilter(Handler handler)
        : m_handler(std::move(handler))
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        Q_UNUSED(watched)

        if (event->type() != QEvent::KeyPress) {
            return false;
        }

        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_unknown) {
            return false;
        }

        if (m_handler(QKeySequence(keyEvent->keyCombination()))) {
            keyEvent->accept();
            return true;
        }
        return false;
    }

private:
    Handler m_handler;
};
}

namespace kiriview::ApplicationActions {
ApplicationShortcutRuntime::ApplicationShortcutRuntime(ApplicationActionHost &host,
    const ApplicationActionRegistry &actionRegistry, ChangeCallback changeCallback,
    TriggerCallbacks triggerCallbacks)
    : m_host(host)
    , m_actionRegistry(actionRegistry)
    , m_changeCallback(std::move(changeCallback))
    , m_triggerCallbacks(std::move(triggerCallbacks))
    , m_shortcutRouteModel(std::make_unique<ShortcutRouteModel>(m_host.actionContext()))
{
}

ApplicationShortcutRuntime::~ApplicationShortcutRuntime() { clearShortcutRouter(); }

void ApplicationShortcutRuntime::setup()
{
    loadViewerLocalShortcuts();
    sanitizeProgramWideActionShortcuts();
    m_shortcutHelpModel = std::make_unique<ShortcutHelpModel>(
        [this]() { return shortcutHelpRows(); }, m_host.actionContext());
}

void ApplicationShortcutRuntime::handleActionChanged(QAction *changedAction)
{
    if (m_sanitizingShortcuts) {
        return;
    }

    if (changedAction != nullptr) {
        sanitizeProgramWideActionShortcuts(changedAction);
    }

    notifyShortcutRowsChanged();
}

void ApplicationShortcutRuntime::setActionStateInput(const ApplicationActionStateInput &input)
{
    m_actionStateInput = input;
    updateShortcutEnabledStates();
}

void ApplicationShortcutRuntime::setShortcutHost(QObject *host)
{
    if (m_shortcutHost == host) {
        return;
    }

    clearShortcutRouter();
    m_shortcutHost = host;
    m_shortcutWindow = shortcutWindow(host);
    rebuildShortcutRouter();
}

int ApplicationShortcutRuntime::shortcutRevision() const { return m_shortcutRevision; }

QAbstractListModel *ApplicationShortcutRuntime::shortcutHelpModel() const
{
    return m_shortcutHelpModel.get();
}

QAbstractListModel *ApplicationShortcutRuntime::shortcutRouteModel() const
{
    return m_shortcutRouteModel.get();
}

void ApplicationShortcutRuntime::loadViewerLocalShortcuts()
{
    KConfigGroup group(KSharedConfig::openConfig(), viewerLocalShortcutsGroup);
    for (const ActionDefinition &definition : definitions()) {
        const std::optional<std::size_t> index = actionIndex(definition.actionId);
        if (!index.has_value()) {
            continue;
        }

        const QString actionKey = QString::fromLatin1(definition.name);
        const QStringList storedTexts = group.readEntry(actionKey, QStringList());
        m_viewerLocalShortcuts[*index] = storedTexts.isEmpty()
            ? defaultShortcuts(definition.defaultViewerLocalShortcuts)
            : shortcutsFromPortableTexts(storedTexts);
    }
}

void ApplicationShortcutRuntime::persistViewerLocalShortcuts(ActionId actionId) const
{
    const ActionDefinition *definition = definitionForId(actionId);
    const std::optional<std::size_t> index = actionIndex(actionId);
    if (definition == nullptr || !index.has_value()) {
        return;
    }

    KConfigGroup group(KSharedConfig::openConfig(), viewerLocalShortcutsGroup);
    group.writeEntry(QString::fromLatin1(definition->name),
        portableShortcutTexts(m_viewerLocalShortcuts[*index]));
    group.sync();
}

void ApplicationShortcutRuntime::notifyShortcutRowsChanged()
{
    if (m_shortcutHelpModel != nullptr) {
        m_shortcutHelpModel->handleRowsChanged();
    }
    ++m_shortcutRevision;
    rebuildShortcutRouter();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void ApplicationShortcutRuntime::sanitizeProgramWideActionShortcuts()
{
    for (QAction *action : m_host.mainActionCollection()->actions()) {
        sanitizeProgramWideActionShortcuts(action);
    }
}

void ApplicationShortcutRuntime::sanitizeProgramWideActionShortcuts(QAction *action)
{
    if (action == nullptr) {
        return;
    }

    const QList<QKeySequence> shortcuts = action->shortcuts();
    const QList<QKeySequence> sanitizedShortcuts = sanitizeProgramWideShortcuts(shortcuts);
    if (sanitizedShortcuts == shortcuts) {
        return;
    }

    m_sanitizingShortcuts = true;
    action->setShortcuts(sanitizedShortcuts);
    m_host.mainActionCollection()->writeSettings(nullptr, false, action);
    m_sanitizingShortcuts = false;
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForAction(
    ActionId actionId, const QAction *action) const
{
    return ApplicationActions::shortcutProjection(
        action == nullptr ? QList<QKeySequence>() : action->shortcuts(),
        viewerLocalShortcutsForId(actionId));
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjection(
    const QString &actionName) const
{
    const ActionDefinition *definition = definitionForName(actionName);
    return shortcutProjectionForAction(
        definition == nullptr ? ActionId::ActionCount : definition->actionId,
        m_actionRegistry.action(actionName));
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForId(
    ActionId actionId) const
{
    const ActionDefinition *definition = definitionForId(actionId);
    return shortcutProjectionForAction(
        definition == nullptr ? ActionId::ActionCount : definition->actionId,
        m_actionRegistry.actionForId(actionId));
}

QList<QKeySequence> ApplicationShortcutRuntime::programWideShortcuts(
    const QString &actionName) const
{
    QAction *action = m_actionRegistry.action(actionName);
    return action == nullptr ? QList<QKeySequence>() : action->shortcuts();
}

QList<QKeySequence> ApplicationShortcutRuntime::programWideShortcutsForId(ActionId actionId) const
{
    QAction *action = m_actionRegistry.actionForId(actionId);
    return action == nullptr ? QList<QKeySequence>() : action->shortcuts();
}

QList<QKeySequence> ApplicationShortcutRuntime::viewerLocalShortcuts(
    const QString &actionName) const
{
    const ActionDefinition *definition = definitionForName(actionName);
    return definition == nullptr ? QList<QKeySequence>()
                                 : viewerLocalShortcutsForId(definition->actionId);
}

QList<QKeySequence> ApplicationShortcutRuntime::viewerLocalShortcutsForId(ActionId actionId) const
{
    const std::optional<std::size_t> index = actionIndex(actionId);
    return index.has_value() ? m_viewerLocalShortcuts[*index] : QList<QKeySequence>();
}

bool ApplicationShortcutRuntime::setViewerLocalShortcuts(
    const QString &actionName, const QList<QKeySequence> &shortcuts)
{
    const ActionDefinition *definition = definitionForName(actionName);
    return definition != nullptr && setViewerLocalShortcutsForId(definition->actionId, shortcuts);
}

bool ApplicationShortcutRuntime::setViewerLocalShortcutsForId(
    ActionId actionId, const QList<QKeySequence> &shortcuts)
{
    const std::optional<std::size_t> index = actionIndex(actionId);
    if (!index.has_value()) {
        return false;
    }

    m_viewerLocalShortcuts[*index] = shortcuts;
    persistViewerLocalShortcuts(actionId);
    notifyShortcutRowsChanged();
    return true;
}

void ApplicationShortcutRuntime::clearShortcutRouter()
{
    if (m_shortcutEventFilter != nullptr) {
        if (QCoreApplication::instance() != nullptr) {
            QCoreApplication::instance()->removeEventFilter(m_shortcutEventFilter.get());
        }
        if (m_shortcutHost != nullptr) {
            m_shortcutHost->removeEventFilter(m_shortcutEventFilter.get());
        }
        if (m_shortcutWindow != nullptr && m_shortcutWindow != m_shortcutHost) {
            m_shortcutWindow->removeEventFilter(m_shortcutEventFilter.get());
        }
    }
    m_shortcutEventFilter.reset();
    m_shortcuts.clear();
}

void ApplicationShortcutRuntime::rebuildShortcutRouter()
{
    m_shortcuts.clear();
    if (m_shortcutHost == nullptr) {
        return;
    }

    if (m_shortcutEventFilter == nullptr) {
        m_shortcutEventFilter = std::make_unique<ShortcutEventFilter>(
            [this](const QKeySequence &shortcut) { return handleShortcutEvent(shortcut); });
        if (QCoreApplication::instance() != nullptr) {
            QCoreApplication::instance()->installEventFilter(m_shortcutEventFilter.get());
        }
        m_shortcutHost->installEventFilter(m_shortcutEventFilter.get());
        if (m_shortcutWindow != nullptr && m_shortcutWindow != m_shortcutHost) {
            m_shortcutWindow->installEventFilter(m_shortcutEventFilter.get());
        }
    }

    for (const ApplicationShortcutRoute &route : shortcutRoutes()) {
        for (ActionId actionId : route.actionIds) {
            const QList<QKeySequence> shortcuts
                = route.activationScope == ApplicationShortcutActivationScope::ProgramWide
                ? programWideShortcutsForId(actionId)
                : viewerLocalShortcutsForId(actionId);
            addShortcutBinding(actionId, shortcuts, route.activationScope, route.shortcutScope);
        }
    }

    addShortcutBinding(ActionId::OpenApplicationMenuAction,
        programWideShortcutsForId(ActionId::OpenApplicationMenuAction),
        ApplicationShortcutActivationScope::ProgramWide);
    addShortcutBinding(ActionId::OptionsShowMenubarAction,
        programWideShortcutsForId(ActionId::OptionsShowMenubarAction),
        ApplicationShortcutActivationScope::ProgramWide);
    updateShortcutEnabledStates();
}

void ApplicationShortcutRuntime::addShortcutBinding(ActionId actionId,
    const QList<QKeySequence> &shortcuts, ApplicationShortcutActivationScope activationScope,
    std::optional<ImageShortcutScope> shortcutScope)
{
    if (shortcuts.isEmpty() || m_shortcutHost == nullptr) {
        return;
    }

    m_shortcuts.push_back(
        ShortcutBinding { actionId, activationScope, shortcutScope, shortcuts, false });
}

bool ApplicationShortcutRuntime::handleShortcutEvent(const QKeySequence &shortcut)
{
    if (shortcut.isEmpty()) {
        return false;
    }

    const bool focusApplicable
        = m_shortcutWindow == nullptr || QGuiApplication::focusWindow() == m_shortcutWindow;

    if (handleFixedShortcutEvent(shortcut)) {
        return true;
    }

    GenericShortcutDispatchInput input;
    input.focusApplicable = focusApplicable;
    input.actionState = m_actionStateInput;
    input.bindings.reserve(static_cast<qsizetype>(m_shortcuts.size()));
    for (const ShortcutBinding &binding : m_shortcuts) {
        input.bindings.push_back(GenericShortcutBinding {
            binding.actionId,
            binding.shortcutScope,
            binding.shortcuts,
            actionEnabledForShortcut(binding.actionId),
        });
    }

    const GenericShortcutDispatchOutcome outcome = genericShortcutDispatchOutcome(input, shortcut);
    switch (outcome.kind) {
    case GenericShortcutDispatchKind::UnsupportedVideoAction:
        if (m_triggerCallbacks.unsupportedVideoActionTriggered) {
            m_triggerCallbacks.unsupportedVideoActionTriggered(outcome.actionId);
        }
        return true;
    case GenericShortcutDispatchKind::UnsupportedImageAction:
        if (m_triggerCallbacks.unsupportedImageActionTriggered) {
            m_triggerCallbacks.unsupportedImageActionTriggered(outcome.actionId);
        }
        return true;
    case GenericShortcutDispatchKind::TriggerAction:
        if (QAction *action = m_actionRegistry.actionForId(outcome.actionId);
            action != nullptr && action->isEnabled()) {
            action->trigger();
        }
        return true;
    case GenericShortcutDispatchKind::None:
        return false;
    }

    return false;
}

bool ApplicationShortcutRuntime::handleFixedShortcutEvent(const QKeySequence &shortcut)
{
    const FixedShortcutDispatchInput input {
        m_shortcutWindow == nullptr || QGuiApplication::focusWindow() == m_shortcutWindow,
        m_actionStateInput.videoMode,
        m_actionStateInput.helpActionsEnabled,
        m_actionStateInput.viewerShortcutsEnabled,
        m_actionStateInput.readyViewerShortcutsEnabled,
        m_actionStateInput.videoFileDeletionInProgress,
        m_actionStateInput.activeNavigationActionsAvailable,
        m_actionStateInput.twoPageViewerShortcutsEnabled,
        m_actionStateInput.pannableViewerShortcutsEnabled,
    };

    const FixedShortcutDispatchOutcome outcome = fixedShortcutDispatchOutcome(input, shortcut);
    switch (outcome.kind) {
    case FixedShortcutDispatchKind::VideoSeek:
        return m_triggerCallbacks.videoSeekShortcutTriggered
            && m_triggerCallbacks.videoSeekShortcutTriggered(outcome.videoSeekDeltaMilliseconds);
    case FixedShortcutDispatchKind::HorizontalArrow:
        return m_triggerCallbacks.horizontalArrowShortcutTriggered
            && m_triggerCallbacks.horizontalArrowShortcutTriggered(outcome.previousOrUp);
    case FixedShortcutDispatchKind::SinglePageArrow:
        return m_triggerCallbacks.singlePageArrowShortcutTriggered
            && m_triggerCallbacks.singlePageArrowShortcutTriggered(outcome.previousOrUp);
    case FixedShortcutDispatchKind::VerticalPan:
        return m_triggerCallbacks.verticalPanShortcutTriggered
            && m_triggerCallbacks.verticalPanShortcutTriggered(outcome.previousOrUp);
    case FixedShortcutDispatchKind::None:
        return false;
    }

    return false;
}

bool ApplicationShortcutRuntime::actionEnabledForShortcut(ActionId actionId) const
{
    const QAction *action = m_actionRegistry.actionForId(actionId);
    return action != nullptr && action->isEnabled();
}

void ApplicationShortcutRuntime::updateShortcutEnabledStates()
{
    for (ShortcutBinding &binding : m_shortcuts) {
        binding.enabled = genericShortcutBindingEnabled(m_actionStateInput,
            GenericShortcutBinding {
                binding.actionId,
                binding.shortcutScope,
                binding.shortcuts,
                actionEnabledForShortcut(binding.actionId),
            });
    }
}

QString ApplicationShortcutRuntime::actionDisplayText(const QAction *action)
{
    if (action == nullptr) {
        return {};
    }

    const QString text = action->text();
    QString displayText;
    displayText.reserve(text.size());
    for (qsizetype index = 0; index < text.size(); ++index) {
        if (text.at(index) != QLatin1Char('&')) {
            displayText.append(text.at(index));
            continue;
        }

        if (index + 1 < text.size() && text.at(index + 1) == QLatin1Char('&')) {
            displayText.append(QLatin1Char('&'));
            ++index;
        }
    }

    return displayText;
}

QString ApplicationShortcutRuntime::shortcutDisplayText(const QAction *action)
{
    return shortcutKeyDisplayTexts(action).join(QStringLiteral(" / "));
}

QStringList ApplicationShortcutRuntime::shortcutKeyDisplayTexts(const QAction *action)
{
    QStringList texts;
    if (action != nullptr) {
        const QList<QKeySequence> shortcuts = action->shortcuts();
        texts.reserve(shortcuts.size());
        for (const QKeySequence &shortcut : shortcuts) {
            if (!shortcut.isEmpty()) {
                texts.push_back(shortcut.toString(QKeySequence::NativeText));
            }
        }
    }

    if (texts.isEmpty()) {
        texts.push_back(i18nc("@info:keyboard shortcut", "Unassigned"));
    }

    return texts;
}

QString shortcutScopeText(ApplicationShortcutActivationScope scope)
{
    switch (scope) {
    case ApplicationShortcutActivationScope::ProgramWide:
        return i18nc("@info:shortcut scope", "Program-wide");
    case ApplicationShortcutActivationScope::ViewerLocal:
        return i18nc("@info:shortcut scope", "Viewer-local");
    }

    return {};
}

QStringList shortcutKeyDisplayTextsForList(const QList<QKeySequence> &shortcuts)
{
    QStringList texts;
    texts.reserve(shortcuts.size());
    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcut.isEmpty()) {
            texts.push_back(shortcut.toString(QKeySequence::NativeText));
        }
    }

    if (texts.isEmpty()) {
        texts.push_back(i18nc("@info:keyboard shortcut", "Unassigned"));
    }

    return texts;
}

QList<ShortcutHelpRow> ApplicationShortcutRuntime::shortcutHelpRows() const
{
    QList<ShortcutHelpRow> rows;
    rows.reserve(static_cast<qsizetype>(definitions().size()));

    for (const RegisteredApplicationAction &registeredAction :
        m_actionRegistry.registeredActions()) {
        if (!KirigamiActionCollection::isShortcutsConfigurable(registeredAction.action)) {
            continue;
        }

        const ShortcutHelpCategory category = shortcutHelpCategory(registeredAction.actionId);
        QString actionText = actionDisplayText(registeredAction.action);
        if (actionText.isEmpty()) {
            if (const ActionDefinition *definition = definitionForId(registeredAction.actionId)) {
                actionText = localizedString(definition->text);
            }
        }
        const auto appendRow
            = [&](ApplicationShortcutActivationScope scope, const QList<QKeySequence> &shortcuts) {
                  if (shortcuts.isEmpty()) {
                      return;
                  }
                  const QStringList keyTexts = shortcutKeyDisplayTextsForList(shortcuts);
                  rows.push_back(ShortcutHelpRow {
                      static_cast<int>(registeredAction.actionId),
                      registeredAction.actionName,
                      actionText,
                      keyTexts.join(QStringLiteral(" / ")),
                      shortcutHelpCategoryKey(category),
                      shortcutHelpCategoryText(category),
                      shortcutScopeText(scope),
                      keyTexts,
                  });
              };

        appendRow(ApplicationShortcutActivationScope::ProgramWide,
            registeredAction.action == nullptr ? QList<QKeySequence>()
                                               : registeredAction.action->shortcuts());
        appendRow(ApplicationShortcutActivationScope::ViewerLocal,
            viewerLocalShortcutsForId(registeredAction.actionId));
    }

    std::stable_sort(
        rows.begin(), rows.end(), [](const ShortcutHelpRow &left, const ShortcutHelpRow &right) {
            const int leftOrder
                = shortcutHelpCategoryOrderForAction(static_cast<ActionId>(left.actionId));
            const int rightOrder
                = shortcutHelpCategoryOrderForAction(static_cast<ActionId>(right.actionId));
            if (leftOrder != rightOrder) {
                return leftOrder < rightOrder;
            }
            if (left.actionId != right.actionId) {
                return left.actionId < right.actionId;
            }
            return left.scopeText < right.scopeText;
        });

    for (qsizetype index = 0; index < rows.size(); ++index) {
        rows[index].categoryFirst
            = index == 0 || rows.at(index - 1).categoryKey != rows.at(index).categoryKey;
        rows[index].categoryLast = index == rows.size() - 1
            || rows.at(index + 1).categoryKey != rows.at(index).categoryKey;
    }

    return rows;
}
}
