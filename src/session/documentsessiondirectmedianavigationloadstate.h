// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONLOADSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONLOADSTATE_H

#include "async/imageasyncoperationstate.h"
#include "session/directmediacursor.h"

#include <QtGlobal>

namespace kiriview {
struct DocumentSessionDirectMediaNavigationLoad {
    quint64 operationId = 0;
    DirectMediaScope scope;
};

class DocumentSessionDirectMediaNavigationLoadState final
{
public:
    explicit DocumentSessionDirectMediaNavigationLoadState(quint64 nextOperationId = 0);

    DocumentSessionDirectMediaNavigationLoad start(const DirectMediaScope &scope);
    bool accepts(const DocumentSessionDirectMediaNavigationLoad &load) const;
    bool finish(const DocumentSessionDirectMediaNavigationLoad &load);
    void cancel();

private:
    ImageAsyncScopedOperationState<DirectMediaScope> m_operation;
};
}

#endif
