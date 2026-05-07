// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "localization.h"

#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <QByteArray>
#include <QQmlApplicationEngine>

#ifndef TRANSLATION_DOMAIN
#define TRANSLATION_DOMAIN "kiriview"
#endif

namespace KiriView {
void initializeLocalization()
{
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
}

void setupLocalizedContext(QQmlApplicationEngine &engine)
{
    KLocalization::setupLocalizedContext(&engine);
}
}
