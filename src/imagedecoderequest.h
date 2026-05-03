// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEREQUEST_H
#define KIRIVIEW_IMAGEDECODEREQUEST_H

#include "imagelocation.h"

#include <QUrl>
#include <QtGlobal>

namespace KiriView {
struct ImageDecodeRequest {
    quint64 id = 0;
    QUrl imageUrl;
    ArchiveDocumentLocation archiveDocument;
};
}

#endif
