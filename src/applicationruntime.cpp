// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationruntime.h"

#include "localization.h"

#include <QGuiApplication>
#include <QQuickStyle>
#include <QString>
#include <QtGlobal>

namespace {
void setupApplicationIdentity()
{
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.hnjae.KiriView"));
}

void setupDefaultQuickStyle()
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}
}

namespace KiriView {
void initializeApplicationRuntime()
{
    initializeLocalization();
    setupApplicationIdentity();
    setupDefaultQuickStyle();
}
}
