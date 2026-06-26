// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONCOMPLETIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONCOMPLETIONRUNTIME_H

#include "session/documentsessionmediadeletionruntime.h"
#include "session/documentsessionrouteplan.h"

#include <QString>
#include <functional>

namespace kiriview {
struct DocumentSessionMediaDeletionCompletionRuntimePorts
{
    std::function<void(bool)> setFileDeletionInProgress;
    std::function<void(const QString&)> setSessionErrorString;
    std::function<void()> recomputePublicProjection;
    std::function<void(const DocumentSessionRoutePlan&)> executeRoutePlan;
};

class DocumentSessionMediaDeletionCompletionRuntime final
{
public:
    explicit DocumentSessionMediaDeletionCompletionRuntime(
        DocumentSessionMediaDeletionCompletionRuntimePorts ports = {});

    void apply(const DocumentSessionMediaDeletionCompletion& completion);

private:
    DocumentSessionMediaDeletionCompletionRuntimePorts m_ports;
};
}

#endif
