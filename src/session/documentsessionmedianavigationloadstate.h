// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIANAVIGATIONLOADSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIANAVIGATIONLOADSTATE_H

#include "async/imageasyncoperationstate.h"

#include <QUrl>
#include <QtGlobal>

namespace KiriView {
struct DocumentSessionMediaNavigationLoadScope {
    QUrl currentUrl;
    QUrl parentUrl;
    quint64 cursorGeneration = 0;

    friend bool operator==(const DocumentSessionMediaNavigationLoadScope &left,
        const DocumentSessionMediaNavigationLoadScope &right)
    {
        return left.currentUrl == right.currentUrl && left.parentUrl == right.parentUrl
            && left.cursorGeneration == right.cursorGeneration;
    }
};

struct DocumentSessionMediaNavigationLoad {
    quint64 operationId = 0;
    QUrl currentUrl;
    QUrl parentUrl;
    quint64 cursorGeneration = 0;
};

class DocumentSessionMediaNavigationLoadState final
{
public:
    explicit DocumentSessionMediaNavigationLoadState(quint64 nextOperationId = 0);

    DocumentSessionMediaNavigationLoad start(
        const QUrl &currentUrl, const QUrl &parentUrl, quint64 cursorGeneration);
    bool accepts(const DocumentSessionMediaNavigationLoad &load) const;
    bool finish(const DocumentSessionMediaNavigationLoad &load);
    void cancel();

private:
    ImageAsyncScopedOperationState<DocumentSessionMediaNavigationLoadScope> m_operation;
};
}

#endif
