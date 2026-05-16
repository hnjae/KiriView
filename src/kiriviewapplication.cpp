// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"

#include "kiriviewapplicationactions.h"
#include "kiriviewstate.h"

#include <KLocalizedString>
#include <KirigamiActionCollection>
#include <QAbstractListModel>
#include <QIcon>
#include <QSignalBlocker>
#include <QVariant>

namespace {
namespace Actions = KiriView::ApplicationActions;

class ShortcutHelpModel : public QAbstractListModel
{
public:
    enum Role {
        ActionIdRole = Qt::UserRole + 1,
        ActionNameRole,
        ActionTextRole,
        ShortcutTextRole,
    };

    explicit ShortcutHelpModel(KiriViewApplication &application, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_application(application)
        , m_rows(collectRows())
    {
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
            return {};
        }

        const Row &row = m_rows.at(index.row());
        switch (role) {
        case ActionIdRole:
            return static_cast<int>(row.actionId);
        case ActionNameRole:
            return row.actionName;
        case ActionTextRole:
            return actionDisplayText(row.action);
        case ShortcutTextRole:
            return shortcutDisplayText(row.action);
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            { ActionIdRole, QByteArrayLiteral("actionId") },
            { ActionNameRole, QByteArrayLiteral("actionName") },
            { ActionTextRole, QByteArrayLiteral("actionText") },
            { ShortcutTextRole, QByteArrayLiteral("shortcutText") },
        };
    }

    void handleActionChanged(QAction *changedAction)
    {
        const QList<Row> rows = collectRows();
        if (!sameRows(rows, m_rows)) {
            beginResetModel();
            m_rows = rows;
            endResetModel();
            return;
        }

        if (changedAction != nullptr) {
            for (int row = 0; row < m_rows.size(); ++row) {
                if (m_rows.at(row).action == changedAction) {
                    Q_EMIT dataChanged(
                        index(row, 0), index(row, 0), { ActionTextRole, ShortcutTextRole });
                    return;
                }
            }
        }

        if (!m_rows.isEmpty()) {
            Q_EMIT dataChanged(
                index(0, 0), index(m_rows.size() - 1, 0), { ActionTextRole, ShortcutTextRole });
        }
    }

private:
    struct Row {
        QAction *action = nullptr;
        KiriViewApplication::ActionId actionId = KiriViewApplication::ActionCount;
        QString actionName;
    };

    QList<Row> collectRows() const
    {
        QList<Row> rows;
        rows.reserve(static_cast<qsizetype>(Actions::definitions().size()));

        for (const Actions::ActionDefinition &definition : Actions::definitions()) {
            const QString name = QString::fromLatin1(definition.name);
            QAction *action = m_application.action(name);
            if (action == nullptr || !KirigamiActionCollection::isShortcutsConfigurable(action)) {
                continue;
            }

            rows.push_back(Row { action, definition.actionId, name });
        }

        return rows;
    }

    static bool sameRows(const QList<Row> &left, const QList<Row> &right)
    {
        if (left.size() != right.size()) {
            return false;
        }

        for (int index = 0; index < left.size(); ++index) {
            if (left.at(index).action != right.at(index).action
                || left.at(index).actionId != right.at(index).actionId
                || left.at(index).actionName != right.at(index).actionName) {
                return false;
            }
        }

        return true;
    }

    static QString actionDisplayText(const QAction *action)
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

    static QString shortcutDisplayText(const QAction *action)
    {
        if (action == nullptr) {
            return {};
        }

        const QString text = Actions::shortcutListText(action->shortcuts());
        return text.isEmpty() ? i18nc("@info:keyboard shortcut", "Unassigned") : text;
    }

    KiriViewApplication &m_application;
    QList<Row> m_rows;
};

KiriViewApplication::MenuPresentation toMenuPresentation(int value)
{
    if (value == static_cast<int>(KiriViewApplication::MenuBar)) {
        return KiriViewApplication::MenuBar;
    }

    return KiriViewApplication::HamburgerMenu;
}

}

KiriViewApplication::KiriViewApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
{
    KiriViewApplication::setupActions();
    m_shortcutHelpModel = new ShortcutHelpModel(*this, this);
}

KiriViewApplication::~KiriViewApplication() = default;

KiriViewApplication::MenuPresentation KiriViewApplication::menuPresentation() const
{
    return toMenuPresentation(KiriViewState::menuPresentation());
}

void KiriViewApplication::setMenuPresentation(MenuPresentation presentation)
{
    if (toMenuPresentation(KiriViewState::menuPresentation()) == presentation) {
        return;
    }

    KiriViewState::setMenuPresentation(static_cast<int>(presentation));
    KiriViewState::self()->save();
    updateShowMenuBarAction();
    Q_EMIT menuPresentationChanged();
}

int KiriViewApplication::shortcutRevision() const { return m_shortcutRevision; }

QAbstractListModel *KiriViewApplication::shortcutHelpModel() const { return m_shortcutHelpModel; }

QAction *KiriViewApplication::action(const QString &actionName)
{
    return AbstractKirigamiApplication::action(actionName);
}

QAction *KiriViewApplication::actionForId(ActionId actionId)
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return nullptr;
    }

    return action(name);
}

QString KiriViewApplication::actionName(ActionId actionId) const
{
    return Actions::actionName(actionId);
}

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    const QAction *registeredAction = const_cast<KiriViewApplication *>(this)->action(actionName);
    if (!registeredAction) {
        return {};
    }

    return registeredAction->shortcuts();
}

QList<QKeySequence> KiriViewApplication::shortcutsForId(ActionId actionId) const
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return {};
    }

    return shortcuts(name);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return Actions::filterShortcutsByCommandModifier(shortcuts(actionName), true);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifierForId(ActionId actionId) const
{
    return Actions::filterShortcutsByCommandModifier(shortcutsForId(actionId), true);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return Actions::filterShortcutsByCommandModifier(shortcuts(actionName), false);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifierForId(
    ActionId actionId) const
{
    return Actions::filterShortcutsByCommandModifier(shortcutsForId(actionId), false);
}

QList<QKeySequence> KiriViewApplication::shortcutAliases(const QString &actionName) const
{
    return Actions::shortcutAliases(shortcuts(actionName));
}

QList<QKeySequence> KiriViewApplication::shortcutAliasesForId(ActionId actionId) const
{
    return Actions::shortcutAliases(shortcutsForId(actionId));
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return Actions::shortcutListText(shortcuts(actionName));
}

QString KiriViewApplication::shortcutTextForId(ActionId actionId) const
{
    return Actions::shortcutListText(shortcutsForId(actionId));
}

QKeySequence KiriViewApplication::menuShortcut(const QString &actionName) const
{
    return Actions::menuShortcut(shortcuts(actionName));
}

QKeySequence KiriViewApplication::menuShortcutForId(ActionId actionId) const
{
    return Actions::menuShortcut(shortcutsForId(actionId));
}

QString KiriViewApplication::menuShortcutText(const QString &actionName) const
{
    return menuShortcut(actionName).toString(QKeySequence::NativeText);
}

QString KiriViewApplication::menuShortcutTextForId(ActionId actionId) const
{
    return menuShortcutForId(actionId).toString(QKeySequence::NativeText);
}

void KiriViewApplication::setupActions()
{
    AbstractKirigamiApplication::setupActions();
    mainCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

    const auto addAction = [this](const Actions::ActionDefinition &definition) {
        const QString name = QString::fromLatin1(definition.name);
        const QList<QKeySequence> shortcuts
            = Actions::defaultShortcuts(definition.defaultShortcuts);

        switch (definition.kind) {
        case Actions::RegistrationKind::Existing:
            if (QAction *registeredAction = action(name)) {
                finishRegisteredAction(registeredAction, registeredAction->text(), shortcuts);
            }
            return;
        case Actions::RegistrationKind::Inherited:
            return;
        case Actions::RegistrationKind::Registered:
            addRegisteredAction(name, Actions::localizedString(definition.text),
                Actions::latin1String(definition.iconName), shortcuts);
            return;
        case Actions::RegistrationKind::ShowMenubar:
            m_showMenuBarAction = addStandardAction(
                definition.actionType, name, Actions::localizedString(definition.text), shortcuts);
            KirigamiActionCollection::setShortcutsConfigurable(m_showMenuBarAction, false);
            m_showMenuBarAction->setCheckable(true);
            connect(m_showMenuBarAction, &QAction::triggered, this,
                [this](bool checked) { setMenuPresentation(checked ? MenuBar : HamburgerMenu); });
            return;
        case Actions::RegistrationKind::Standard:
            addStandardAction(
                definition.actionType, name, Actions::localizedString(definition.text), shortcuts);
            return;
        }
    };

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        addAction(definition);
    }

    readSettings();
    sanitizeActionShortcuts();
    updateShowMenuBarAction();
}

QAction *KiriViewApplication::addRegisteredAction(const QString &name, const QString &text,
    const QString &iconName, const QList<QKeySequence> &defaultShortcuts)
{
    auto *registeredAction = new QAction(this);
    registeredAction->setObjectName(name);
    registeredAction->setText(text);
    if (!iconName.isEmpty()) {
        registeredAction->setIcon(QIcon::fromTheme(iconName));
    }

    mainCollection()->addAction(name, registeredAction);
    KirigamiActionCollection::setDefaultShortcuts(registeredAction, defaultShortcuts);
    connect(registeredAction, &QAction::changed, this, &KiriViewApplication::handleActionChanged);
    return registeredAction;
}

QAction *KiriViewApplication::addStandardAction(KStandardActions::StandardAction actionType,
    const QString &name, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    QAction *registeredAction = mainCollection()->addAction(actionType, name, this, [](bool) { });
    return finishRegisteredAction(registeredAction, text, defaultShortcuts);
}

QAction *KiriViewApplication::finishRegisteredAction(
    QAction *registeredAction, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    registeredAction->setText(text);
    KirigamiActionCollection::setDefaultShortcuts(registeredAction, defaultShortcuts);
    connect(registeredAction, &QAction::changed, this, &KiriViewApplication::handleActionChanged);
    return registeredAction;
}

void KiriViewApplication::handleActionChanged()
{
    if (m_sanitizingShortcuts) {
        return;
    }

    auto *changedAction = qobject_cast<QAction *>(sender());
    if (changedAction != nullptr) {
        sanitizeActionShortcuts(changedAction);
    }

    if (m_shortcutHelpModel != nullptr) {
        static_cast<ShortcutHelpModel *>(m_shortcutHelpModel)->handleActionChanged(changedAction);
    }
    ++m_shortcutRevision;
    Q_EMIT shortcutRevisionChanged();
}

void KiriViewApplication::sanitizeActionShortcuts()
{
    for (QAction *registeredAction : mainCollection()->actions()) {
        sanitizeActionShortcuts(registeredAction);
    }
}

void KiriViewApplication::sanitizeActionShortcuts(QAction *action)
{
    if (!action) {
        return;
    }

    const QList<QKeySequence> shortcuts = action->shortcuts();
    const QList<QKeySequence> sanitizedShortcuts = Actions::sanitizeShortcuts(shortcuts);
    if (sanitizedShortcuts == shortcuts) {
        return;
    }

    m_sanitizingShortcuts = true;
    action->setShortcuts(sanitizedShortcuts);
    mainCollection()->writeSettings(nullptr, false, action);
    m_sanitizingShortcuts = false;
}

void KiriViewApplication::updateShowMenuBarAction()
{
    if (!m_showMenuBarAction) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == MenuBar);
}
