// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationactionhost.h"
#include "application/applicationactionregistry.h"
#include "application/kiriviewapplicationactions.h"
#include "kiriviewstate.h"

#include <KirigamiActionCollection>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QStandardPaths>
#include <QTest>
#include <cstddef>
#include <memory>
#include <vector>

namespace {
namespace Actions = KiriView::ApplicationActions;
using ActionId = KiriView::ApplicationActions::ActionId;

QString definitionActionName(const Actions::ActionDefinition &definition)
{
    return QString::fromLatin1(definition.name);
}

void resetConfig()
{
    KiriViewState *state = KiriViewState::self();
    state->config()->deleteGroup(QStringLiteral("Interface"));
    state->config()->sync();
    state->config()->reparseConfiguration();
    state->read();
}

class FakeApplicationActionHost final : public KiriView::ApplicationActions::ApplicationActionHost
{
public:
    FakeApplicationActionHost()
        : collection(&object)
    {
    }

    QObject *actionContext() override { return &object; }
    KirigamiActionCollection *mainActionCollection() override { return &collection; }
    QAction *inheritedAction(const QString &actionName) override
    {
        ++inheritedActionLookupCount;
        return collection.action(actionName);
    }
    void readActionSettings() override { ++readActionSettingsCount; }

    QObject object;
    std::vector<std::unique_ptr<QAction>> actions;
    KirigamiActionCollection collection;
    int inheritedActionLookupCount = 0;
    int readActionSettingsCount = 0;
};

void addHostAction(FakeApplicationActionHost &host, const Actions::ActionDefinition &definition)
{
    const QString actionName = definitionActionName(definition);
    auto action = std::make_unique<QAction>(&host.object);
    action->setObjectName(actionName);
    host.collection.addAction(actionName, action.get());
    host.actions.push_back(std::move(action));
}

void addHostActions(FakeApplicationActionHost &host)
{
    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        addHostAction(host, definition);
    }
}
}

class TestApplicationActionRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void registeredActionsResolveThroughDefinitionIdentity();
    void registeredActionsFollowDefinitionOrder();
    void registryUsesHostInheritedActionLookup();
};

void TestApplicationActionRegistry::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    resetConfig();
}

void TestApplicationActionRegistry::init() { resetConfig(); }

void TestApplicationActionRegistry::registeredActionsResolveThroughDefinitionIdentity()
{
    FakeApplicationActionHost host;
    addHostActions(host);
    Actions::ApplicationActionRegistry registry(host);

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        registry.registerAction(definition, registry.collectionAction(definition));
    }

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        const QString actionName = definitionActionName(definition);
        QAction *action = registry.action(actionName);

        QVERIFY2(
            action != nullptr, qPrintable(QStringLiteral("Missing action %1").arg(actionName)));
        QCOMPARE(registry.actionForId(definition.actionId), action);
        QCOMPARE(registry.actionName(definition.actionId), actionName);
    }

    const auto invalidActionId = static_cast<ActionId>(-1);
    QCOMPARE(registry.actionForId(invalidActionId), nullptr);
    QVERIFY(registry.actionName(invalidActionId).isEmpty());
    QCOMPARE(registry.actionForId(ActionId::ActionCount), nullptr);
    QVERIFY(registry.actionName(ActionId::ActionCount).isEmpty());
    QCOMPARE(registry.action(QStringLiteral("missing_action")), nullptr);
}

void TestApplicationActionRegistry::registeredActionsFollowDefinitionOrder()
{
    FakeApplicationActionHost host;
    addHostActions(host);
    Actions::ApplicationActionRegistry registry(host);

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        registry.registerAction(definition, registry.collectionAction(definition));
    }

    const QList<Actions::RegisteredApplicationAction> registeredActions
        = registry.registeredActions();

    QCOMPARE(registeredActions.size(), static_cast<int>(Actions::definitions().size()));
    for (qsizetype index = 0; index < registeredActions.size(); ++index) {
        const Actions::ActionDefinition &definition
            = Actions::definitions().at(static_cast<std::size_t>(index));
        QCOMPARE(registeredActions.at(index).actionId, definition.actionId);
        QCOMPARE(registeredActions.at(index).actionName, definitionActionName(definition));
        QCOMPARE(registeredActions.at(index).action, registry.actionForId(definition.actionId));
    }
}

void TestApplicationActionRegistry::registryUsesHostInheritedActionLookup()
{
    FakeApplicationActionHost host;
    const Actions::ActionDefinition &definition = Actions::definitions().front();
    const QString actionName = definitionActionName(definition);
    addHostAction(host, definition);

    Actions::ApplicationActionRegistry registry(host);

    QAction *action = host.collection.action(actionName);
    QCOMPARE(registry.collectionAction(definition), action);
    QCOMPARE(host.inheritedActionLookupCount, 1);

    registry.registerAction(definition, registry.collectionAction(definition));
    QCOMPARE(registry.action(actionName), action);
    QCOMPARE(registry.actionForId(definition.actionId), action);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    TestApplicationActionRegistry test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_applicationactionregistry.moc"
