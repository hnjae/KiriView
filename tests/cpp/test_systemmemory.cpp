// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/systemmemory.h"

#include <QObject>
#include <QTest>
#include <optional>
#include <utility>

class TestSystemMemory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapshotUsesInjectedPhysicalMemoryReader();
    void unreadablePhysicalMemoryFallsBackToZero();
    void runtimeDefaultsFillMissingProbeAndPreserveOverrides();
};

void TestSystemMemory::snapshotUsesInjectedPhysicalMemoryReader()
{
    int readCount = 0;
    KiriView::SystemMemoryRuntime runtime;
    runtime.readPhysicalSystemMemory = [&readCount]() -> std::optional<qsizetype> {
        ++readCount;
        return 4096;
    };

    const KiriView::SystemMemorySnapshot snapshot
        = KiriView::systemMemorySnapshot(std::move(runtime));

    QCOMPARE(readCount, 1);
    QCOMPARE(snapshot.physicalByteSize, qsizetype(4096));
    QVERIFY(snapshot.hasPhysicalByteSize());
}

void TestSystemMemory::unreadablePhysicalMemoryFallsBackToZero()
{
    KiriView::SystemMemoryRuntime runtime;
    runtime.readPhysicalSystemMemory = []() -> std::optional<qsizetype> { return std::nullopt; };

    const KiriView::SystemMemorySnapshot snapshot
        = KiriView::systemMemorySnapshot(std::move(runtime));

    QCOMPARE(snapshot.physicalByteSize, qsizetype(0));
    QVERIFY(!snapshot.hasPhysicalByteSize());

    runtime.readPhysicalSystemMemory = []() -> std::optional<qsizetype> { return -128; };

    const KiriView::SystemMemorySnapshot negativeSnapshot
        = KiriView::systemMemorySnapshot(std::move(runtime));

    QCOMPARE(negativeSnapshot.physicalByteSize, qsizetype(0));
    QVERIFY(!negativeSnapshot.hasPhysicalByteSize());
}

void TestSystemMemory::runtimeDefaultsFillMissingProbeAndPreserveOverrides()
{
    int readCount = 0;
    KiriView::SystemMemoryRuntime runtime;
    runtime.readPhysicalSystemMemory = [&readCount]() -> std::optional<qsizetype> {
        ++readCount;
        return 128;
    };

    KiriView::SystemMemoryRuntime resolved
        = KiriView::systemMemoryRuntimeWithDefaults(std::move(runtime));
    QVERIFY(resolved.readPhysicalSystemMemory);
    QCOMPARE(resolved.readPhysicalSystemMemory().value_or(0), qsizetype(128));
    QCOMPARE(readCount, 1);

    resolved = KiriView::systemMemoryRuntimeWithDefaults({});
    QVERIFY(resolved.readPhysicalSystemMemory);
}

QTEST_GUILESS_MAIN(TestSystemMemory)

#include "test_systemmemory.moc"
