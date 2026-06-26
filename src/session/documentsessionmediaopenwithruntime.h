// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIAOPENWITHRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIAOPENWITHRUNTIME_H

#include "async/imageasyncoperationstate.h"
#include "async/imageiojob.h"
#include "session/mediaopenwith.h"

#include <memory>

class QObject;

namespace kiriview {
class DocumentSessionMediaOpenWithRuntime final
{
public:
    explicit DocumentSessionMediaOpenWithRuntime(MediaOpenWithProvider provider = {});
    ~DocumentSessionMediaOpenWithRuntime();

    void open(QObject* receiver, const MediaOpenWithPlan& plan, MediaOpenWithCallback callback);
    void cancel();
    bool active() const;

private:
    MediaOpenWithProvider m_provider;
    ImageIoJob m_job;
    std::shared_ptr<ImageAsyncOperationState> m_operation {
        std::make_shared<ImageAsyncOperationState>()
    };
};
}

#endif
