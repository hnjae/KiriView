// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QQmlComponent>
#include <QTest>

inline bool waitForQmlComponentReady(QQmlComponent &component, int timeoutMs = 10000)
{
    const int intervalMs = 10;
    const int attempts = timeoutMs / intervalMs;

    for (int attempt = 0; component.isLoading() && attempt < attempts; ++attempt) {
        QCoreApplication::processEvents();
        QTest::qWait(intervalMs);
    }

    return !component.isLoading();
}
