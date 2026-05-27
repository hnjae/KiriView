// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIANAVIGATIONLOADSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIANAVIGATIONLOADSTATE_H

#include "async/imageasyncoperationstate.h"
#include "session/directmediacursor.h"

#include <QtGlobal>

namespace KiriView {
struct DocumentSessionMediaNavigationLoad {
    quint64 operationId = 0;
    DirectMediaScope scope;
};

class DocumentSessionMediaNavigationLoadState final
{
public:
    explicit DocumentSessionMediaNavigationLoadState(quint64 nextOperationId = 0);

    DocumentSessionMediaNavigationLoad start(const DirectMediaScope &scope);
    bool accepts(const DocumentSessionMediaNavigationLoad &load) const;
    bool finish(const DocumentSessionMediaNavigationLoad &load);
    void cancel();

private:
    ImageAsyncScopedOperationState<DirectMediaScope> m_operation;
};
}

#endif
