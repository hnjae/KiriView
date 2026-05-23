// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIACANDIDATELOADSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIACANDIDATELOADSTATE_H

#include "async/imageasyncoperationstate.h"

#include <QUrl>
#include <QtGlobal>

namespace KiriView {
struct DocumentSessionMediaCandidateLoadScope {
    QUrl currentUrl;
    QUrl parentUrl;
    quint64 cursorGeneration = 0;

    friend bool operator==(const DocumentSessionMediaCandidateLoadScope &left,
        const DocumentSessionMediaCandidateLoadScope &right)
    {
        return left.currentUrl == right.currentUrl && left.parentUrl == right.parentUrl
            && left.cursorGeneration == right.cursorGeneration;
    }
};

struct DocumentSessionMediaCandidateLoad {
    quint64 operationId = 0;
    QUrl currentUrl;
    QUrl parentUrl;
    quint64 cursorGeneration = 0;
};

class DocumentSessionMediaCandidateLoadState final
{
public:
    explicit DocumentSessionMediaCandidateLoadState(quint64 nextOperationId = 0);

    DocumentSessionMediaCandidateLoad start(
        const QUrl &currentUrl, const QUrl &parentUrl, quint64 cursorGeneration);
    bool accepts(const DocumentSessionMediaCandidateLoad &load) const;
    bool finish(const DocumentSessionMediaCandidateLoad &load);
    void cancel();

private:
    ImageAsyncScopedOperationState<DocumentSessionMediaCandidateLoadScope> m_operation;
};
}

#endif
