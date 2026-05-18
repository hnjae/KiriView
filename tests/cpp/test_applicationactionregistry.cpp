// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionregistry.h"
#include "kiriviewapplication.h"
#include "kiriviewapplicationactions.h"
#include "kiriviewstate.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QStandardPaths>
#include <QTest>
#include <cstddef>

namespace {
namespace Actions = KiriView::ApplicationActions;

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
}

class TestApplicationActionRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void registeredActionsResolveThroughDefinitionIdentity();
    void registeredActionsFollowDefinitionOrder();
};

void TestApplicationActionRegistry::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    resetConfig();
}

void TestApplicationActionRegistry::init() { resetConfig(); }

void TestApplicationActionRegistry::registeredActionsResolveThroughDefinitionIdentity()
{
    KiriViewApplication application;
    Actions::ApplicationActionRegistry registry(application);

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

    const auto invalidActionId = static_cast<KiriViewApplication::ActionId>(-1);
    QCOMPARE(registry.actionForId(invalidActionId), nullptr);
    QVERIFY(registry.actionName(invalidActionId).isEmpty());
    QCOMPARE(registry.actionForId(KiriViewApplication::ActionCount), nullptr);
    QVERIFY(registry.actionName(KiriViewApplication::ActionCount).isEmpty());
    QCOMPARE(registry.action(QStringLiteral("missing_action")), nullptr);
}

void TestApplicationActionRegistry::registeredActionsFollowDefinitionOrder()
{
    KiriViewApplication application;
    Actions::ApplicationActionRegistry registry(application);

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

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    TestApplicationActionRegistry test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_applicationactionregistry.moc"
