// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAOPENWITHTARGET_H
#define KIRIVIEW_MEDIAOPENWITHTARGET_H

#include "location/imagelocation.h"
#include "session/documentsessiontypes.h"

#include <QUrl>

namespace KiriView {
struct MediaOpenWithTargetInput {
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    bool imageReady = false;
    QUrl imageDisplayedUrl;
    OpenedCollectionScopeLocation openedCollectionScope;
    bool videoReady = false;
    QUrl videoSourceUrl;
};

QUrl mediaOpenWithTargetUrl(const MediaOpenWithTargetInput &input);
}

#endif
