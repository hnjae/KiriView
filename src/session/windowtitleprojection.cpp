// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "windowtitleprojection.h"

namespace {
bool validSize(QSize size) { return size.width() > 0 && size.height() > 0; }

QString mediaSizeStatus(QSize size)
{
    if (!validSize(size)) {
        return {};
    }

    return QStringLiteral("%1×%2").arg(size.width()).arg(size.height());
}

QString navigationStatus(kiriview::ActiveNavigationSnapshot snapshot)
{
    if (!snapshot.known || snapshot.currentNumber < 1 || snapshot.count < 1
        || snapshot.currentNumber > snapshot.count) {
        return {};
    }

    return QStringLiteral("%1/%2").arg(snapshot.currentNumber).arg(snapshot.count);
}
}

namespace kiriview {
QString projectWindowTitleSubject(WindowTitleSubjectInput input)
{
    if (input.baseName.isEmpty()) {
        return {};
    }

    QString status;
    switch (input.sourceKind) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        status = mediaSizeStatus(input.directMediaSize);
        break;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        status = navigationStatus(input.activeNavigation);
        break;
    case ActiveNavigationSourceKind::None:
        break;
    }

    if (status.isEmpty()) {
        return input.baseName;
    }

    return QStringLiteral("%1 – %2").arg(input.baseName, status);
}
}
