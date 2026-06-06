// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutruntime.h"

#include "imageactionavailabilitypolicy.h"
#include "shortcuthelpmodel.h"
#include "shortcutroutemodel.h"

#include <KLocalizedString>
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
using ActionId = KiriView::ApplicationActions::ActionId;

KiriView::ApplicationActions::ShortcutHelpCategory shortcutHelpCategory(ActionId actionId)
{
    const KiriView::ApplicationActions::ActionDefinition *definition
        = KiriView::ApplicationActions::definitionForId(actionId);
    if (definition != nullptr) {
        return definition->shortcutHelpCategory;
    }

    return KiriView::ApplicationActions::ShortcutHelpCategory::Help;
}

int shortcutHelpCategoryOrderForAction(ActionId actionId)
{
    return KiriView::ApplicationActions::shortcutHelpCategoryOrder(shortcutHelpCategory(actionId));
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

bool exactShortcut(const QKeySequence &shortcut, const char *portableText)
{
    return shortcut.matches(QKeySequence::fromString(
               QString::fromLatin1(portableText), QKeySequence::PortableText))
        == QKeySequence::ExactMatch;
}

std::optional<qint64> fixedVideoSeekShortcutDeltaMilliseconds(const QKeySequence &shortcut)
{
    if (exactShortcut(shortcut, "Alt+Left")) {
        return -5000;
    }
    if (exactShortcut(shortcut, "Alt+Right")) {
        return 5000;
    }
    if (exactShortcut(shortcut, "Alt+Up")) {
        return 45000;
    }
    if (exactShortcut(shortcut, "Alt+Down")) {
        return -45000;
    }

    return std::nullopt;
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

namespace KiriView::ApplicationActions {
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
    sanitizeActionShortcuts();
    m_shortcutHelpModel = std::make_unique<ShortcutHelpModel>(
        [this]() { return shortcutHelpRows(); }, m_host.actionContext());
}

void ApplicationShortcutRuntime::handleActionChanged(QAction *changedAction)
{
    if (m_sanitizingShortcuts) {
        return;
    }

    if (changedAction != nullptr) {
        sanitizeActionShortcuts(changedAction);
    }

    if (m_shortcutHelpModel != nullptr) {
        m_shortcutHelpModel->handleRowsChanged();
    }
    ++m_shortcutRevision;
    rebuildShortcutRouter();
    if (m_changeCallback) {
        m_changeCallback();
    }
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

void ApplicationShortcutRuntime::sanitizeActionShortcuts()
{
    for (QAction *action : m_host.mainActionCollection()->actions()) {
        sanitizeActionShortcuts(action);
    }
}

void ApplicationShortcutRuntime::sanitizeActionShortcuts(QAction *action)
{
    if (action == nullptr) {
        return;
    }

    const QList<QKeySequence> shortcuts = action->shortcuts();
    const QList<QKeySequence> sanitizedShortcuts = sanitizeShortcuts(shortcuts);
    if (sanitizedShortcuts == shortcuts) {
        return;
    }

    m_sanitizingShortcuts = true;
    action->setShortcuts(sanitizedShortcuts);
    m_host.mainActionCollection()->writeSettings(nullptr, false, action);
    m_sanitizingShortcuts = false;
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForAction(
    const QAction *action, ShortcutAliasPolicy aliasPolicy) const
{
    return ApplicationActions::shortcutProjection(
        action == nullptr ? QList<QKeySequence>() : action->shortcuts(), aliasPolicy);
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjection(
    const QString &actionName) const
{
    const ActionDefinition *definition = definitionForName(actionName);
    return shortcutProjectionForAction(m_actionRegistry.action(actionName),
        definition == nullptr ? ShortcutAliasPolicy::DeriveViewerAlias
                              : definition->shortcutAliasPolicy);
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForId(
    ActionId actionId) const
{
    const ActionDefinition *definition = definitionForId(actionId);
    return shortcutProjectionForAction(m_actionRegistry.actionForId(actionId),
        definition == nullptr ? ShortcutAliasPolicy::DeriveViewerAlias
                              : definition->shortcutAliasPolicy);
}

QList<QKeySequence> ApplicationShortcutRuntime::routedShortcuts(
    ActionId actionId, ApplicationShortcutFilter shortcutFilter) const
{
    const ApplicationShortcutProjection projection = shortcutProjectionForId(actionId);
    switch (shortcutFilter) {
    case ApplicationShortcutFilter::WithCommandModifier:
        return projection.shortcutsWithCommandModifier;
    case ApplicationShortcutFilter::WithoutCommandModifier:
        return projection.shortcutsWithoutCommandModifier;
    case ApplicationShortcutFilter::ShortcutAliases:
        return projection.shortcutAliases;
    case ApplicationShortcutFilter::AllShortcuts:
        return projection.shortcuts;
    }

    return {};
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
            addShortcutBinding(
                actionId, routedShortcuts(actionId, route.shortcutFilter), route.shortcutScope);
        }
    }

    addShortcutBinding(ActionId::OpenApplicationMenuAction,
        routedShortcuts(
            ActionId::OpenApplicationMenuAction, ApplicationShortcutFilter::AllShortcuts));
    addShortcutBinding(ActionId::OptionsShowMenubarAction,
        routedShortcuts(
            ActionId::OptionsShowMenubarAction, ApplicationShortcutFilter::AllShortcuts));
    updateShortcutEnabledStates();
}

void ApplicationShortcutRuntime::addShortcutBinding(ActionId actionId,
    const QList<QKeySequence> &shortcuts, std::optional<ImageShortcutScope> shortcutScope)
{
    if (shortcuts.isEmpty() || m_shortcutHost == nullptr) {
        return;
    }

    m_shortcuts.push_back(ShortcutBinding { actionId, shortcutScope, shortcuts, false });
}

bool ApplicationShortcutRuntime::handleShortcutEvent(const QKeySequence &shortcut)
{
    if (shortcut.isEmpty()) {
        return false;
    }

    if (m_shortcutWindow != nullptr && QGuiApplication::focusWindow() != m_shortcutWindow) {
        return false;
    }

    if (handleFixedShortcutEvent(shortcut)) {
        return true;
    }

    for (const ShortcutBinding &binding : m_shortcuts) {
        if (!binding.enabled || !binding.shortcuts.contains(shortcut)) {
            continue;
        }

        handleShortcutActivated(binding.actionId);
        return true;
    }

    return false;
}

bool ApplicationShortcutRuntime::handleFixedShortcutEvent(const QKeySequence &shortcut)
{
    const VideoShortcutAvailabilityInput videoShortcutInput {
        m_actionStateInput.helpActionsEnabled,
        m_actionStateInput.viewerShortcutsEnabled,
        m_actionStateInput.videoFileDeletionInProgress,
        m_actionStateInput.activeNavigationActionsAvailable,
    };
    const bool horizontalArrowEnabled
        = mediaHorizontalArrowShortcutsEnabled(m_actionStateInput.videoMode,
            m_actionStateInput.readyViewerShortcutsEnabled, videoShortcutInput);
    const bool videoSeekEnabled = m_actionStateInput.videoMode
        && videoShortcutsEnabledForScope(
            videoShortcutInput, ImageShortcutScope::ReadyViewerShortcutScope);
    const std::optional<qint64> videoSeekDelta = fixedVideoSeekShortcutDeltaMilliseconds(shortcut);

    if (videoSeekEnabled && videoSeekDelta.has_value()) {
        return m_triggerCallbacks.videoSeekShortcutTriggered
            && m_triggerCallbacks.videoSeekShortcutTriggered(*videoSeekDelta);
    }
    if (horizontalArrowEnabled && exactShortcut(shortcut, "Left")) {
        return m_triggerCallbacks.horizontalArrowShortcutTriggered
            && m_triggerCallbacks.horizontalArrowShortcutTriggered(true);
    }
    if (horizontalArrowEnabled && exactShortcut(shortcut, "Right")) {
        return m_triggerCallbacks.horizontalArrowShortcutTriggered
            && m_triggerCallbacks.horizontalArrowShortcutTriggered(false);
    }
    if (m_actionStateInput.twoPageViewerShortcutsEnabled && exactShortcut(shortcut, "Shift+Left")) {
        return m_triggerCallbacks.singlePageArrowShortcutTriggered
            && m_triggerCallbacks.singlePageArrowShortcutTriggered(true);
    }
    if (m_actionStateInput.twoPageViewerShortcutsEnabled
        && exactShortcut(shortcut, "Shift+Right")) {
        return m_triggerCallbacks.singlePageArrowShortcutTriggered
            && m_triggerCallbacks.singlePageArrowShortcutTriggered(false);
    }
    if (m_actionStateInput.pannableViewerShortcutsEnabled && exactShortcut(shortcut, "Up")) {
        return m_triggerCallbacks.verticalPanShortcutTriggered
            && m_triggerCallbacks.verticalPanShortcutTriggered(true);
    }
    if (m_actionStateInput.pannableViewerShortcutsEnabled && exactShortcut(shortcut, "Down")) {
        return m_triggerCallbacks.verticalPanShortcutTriggered
            && m_triggerCallbacks.verticalPanShortcutTriggered(false);
    }

    return false;
}

bool ApplicationShortcutRuntime::shortcutBindingEnabled(
    ActionId actionId, std::optional<ImageShortcutScope> shortcutScope) const
{
    const QAction *action = m_actionRegistry.actionForId(actionId);
    const bool actionEnabled = action != nullptr && action->isEnabled();
    const bool unsupportedVideoIntercept
        = m_actionStateInput.videoMode && videoActionUnsupported(actionId);

    if (shortcutScope.has_value()) {
        return applicationShortcutsEnabledForScope(m_actionStateInput, *shortcutScope)
            && (actionEnabled || unsupportedVideoIntercept);
    }

    switch (actionId) {
    case ActionId::OpenApplicationMenuAction:
        return m_actionStateInput.applicationMenuShortcutEnabled && actionEnabled;
    case ActionId::OptionsShowMenubarAction:
        return m_actionStateInput.showMenubarActionEnabled && actionEnabled;
    default:
        return actionEnabled;
    }
}

void ApplicationShortcutRuntime::updateShortcutEnabledStates()
{
    for (ShortcutBinding &binding : m_shortcuts) {
        binding.enabled = shortcutBindingEnabled(binding.actionId, binding.shortcutScope);
    }
}

void ApplicationShortcutRuntime::handleShortcutActivated(ActionId actionId)
{
    if (m_actionStateInput.videoMode && videoActionUnsupported(actionId)) {
        if (m_triggerCallbacks.unsupportedVideoActionTriggered) {
            m_triggerCallbacks.unsupportedVideoActionTriggered(actionId);
        }
        return;
    }

    QAction *action = m_actionRegistry.actionForId(actionId);
    if (action == nullptr || !action->isEnabled()) {
        return;
    }
    action->trigger();
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
        rows.push_back(ShortcutHelpRow {
            static_cast<int>(registeredAction.actionId),
            registeredAction.actionName,
            actionDisplayText(registeredAction.action),
            shortcutDisplayText(registeredAction.action),
            shortcutHelpCategoryKey(category),
            shortcutHelpCategoryText(category),
            shortcutKeyDisplayTexts(registeredAction.action),
        });
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
            return left.actionId < right.actionId;
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
