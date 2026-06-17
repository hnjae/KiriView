// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationactionhost.h"
#include "application/applicationactionruntime.h"
#include "application/applicationactionsourceattachment.h"

#include <KirigamiActionCollection>
#include <QApplication>
#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QTest>

#include <functional>
#include <utility>
#include <vector>

namespace {
namespace Actions = kiriview::ApplicationActions;

class FakeApplicationActionHost final : public Actions::ApplicationActionHost
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
        return collection.action(actionName);
    }
    void readActionSettings() override { }

    QObject object;
    KirigamiActionCollection collection;
};

class FakeActionStateSource final : public QObject, public Actions::ApplicationActionStateSource
{
    Q_OBJECT

public:
    Actions::ApplicationActionStateSnapshot actionStateSnapshot() const override
    {
        return snapshot;
    }

    std::vector<QMetaObject::Connection> connectActionStateChanged(
        QObject *context, std::function<void()> refresh) override
    {
        ++connectCount;
        std::vector<QMetaObject::Connection> connections;
        connections.push_back(QObject::connect(this, &FakeActionStateSource::changed, context,
            [refresh = std::move(refresh)]() { refresh(); }));
        return connections;
    }

    Actions::ApplicationActionStateSnapshot snapshot;
    int connectCount = 0;

Q_SIGNALS:
    void changed();
};
}

class TestApplicationActionSourceAttachment : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceSignalCommitsSnapshotToRuntime();
    void sourceReplacementDisconnectsPreviousSource();
};

void TestApplicationActionSourceAttachment::sourceSignalCommitsSnapshotToRuntime()
{
    FakeApplicationActionHost host;
    Actions::ApplicationActionRuntime runtime(host);
    Actions::ApplicationActionSourceAttachment attachment(runtime, host.object);
    FakeActionStateSource source;

    attachment.setSource(&source);

    QCOMPARE(source.connectCount, 1);
    QCOMPARE(runtime.actionStateRevision(), 1);

    source.snapshot.videoMode = true;
    source.snapshot.imagePannable = true;
    Q_EMIT source.changed();

    const Actions::ApplicationCommandRouterInput input = runtime.commandRouterInput();
    QCOMPARE(runtime.actionStateRevision(), 2);
    QVERIFY(input.videoMode);
    QVERIFY(input.imagePannable);
}

void TestApplicationActionSourceAttachment::sourceReplacementDisconnectsPreviousSource()
{
    FakeApplicationActionHost host;
    Actions::ApplicationActionRuntime runtime(host);
    Actions::ApplicationActionSourceAttachment attachment(runtime, host.object);
    FakeActionStateSource firstSource;
    FakeActionStateSource secondSource;

    attachment.setSource(&firstSource);
    attachment.setSource(&secondSource);
    const int revisionAfterReplacement = runtime.actionStateRevision();

    firstSource.snapshot.videoMode = true;
    Q_EMIT firstSource.changed();

    QCOMPARE(runtime.actionStateRevision(), revisionAfterReplacement);

    secondSource.snapshot.videoMode = true;
    Q_EMIT secondSource.changed();

    QCOMPARE(runtime.actionStateRevision(), revisionAfterReplacement + 1);
    QVERIFY(runtime.commandRouterInput().videoMode);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    TestApplicationActionSourceAttachment test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_applicationactionsourceattachment.moc"
