// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIACANDIDATELOADSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIACANDIDATELOADSTATE_H

#include "async/imageasyncoperationstate.h"

#include <QUrl>
#include <QtGlobal>

namespace KiriView {
struct DocumentSessionMediaCandidateLoad {
    quint64 operationId = 0;
    QUrl currentUrl;
};

class DocumentSessionMediaCandidateLoadState final
{
public:
    explicit DocumentSessionMediaCandidateLoadState(quint64 nextOperationId = 0);

    DocumentSessionMediaCandidateLoad start(const QUrl &currentUrl);
    bool accepts(const DocumentSessionMediaCandidateLoad &load) const;
    bool finish(const DocumentSessionMediaCandidateLoad &load);
    void cancel();

private:
    ImageAsyncOperationState m_operation;
    QUrl m_currentUrl;
};
}

#endif
