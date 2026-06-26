// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediaopenwithruntime.h"

#include "async/imagecallback.h"

#include <QString>
#include <utility>

namespace kiriview {
DocumentSessionMediaOpenWithRuntime::DocumentSessionMediaOpenWithRuntime(
    MediaOpenWithProvider provider)
    : m_provider(mediaOpenWithProviderWithDefault(std::move(provider)))
{
}

DocumentSessionMediaOpenWithRuntime::~DocumentSessionMediaOpenWithRuntime() { cancel(); }

void DocumentSessionMediaOpenWithRuntime::open(
    QObject* receiver, const MediaOpenWithPlan& plan, MediaOpenWithCallback callback)
{
    if (!plan.hasRequest()) {
        invokeIfSet(callback, MediaOpenWithResult::Failed,
            kioOperationValidationFailure(KioOperationKind::MediaOpenWith, QUrl(),
                QStringLiteral("No Open With target is available.")));
        return;
    }

    cancel();
    const std::shared_ptr<ImageAsyncOperationState> operationState = m_operation;
    const quint64 operationId = operationState->start();
    auto sharedCallback = std::make_shared<MediaOpenWithCallback>(std::move(callback));
    m_job = m_provider(receiver, *plan.request,
        [operationState, operationId, sharedCallback](
            MediaOpenWithResult result, const KioOperationFailure& failure) {
            if (!operationState->finish(operationId)) {
                return;
            }

            invokeIfSet(*sharedCallback, result, failure);
        });
}

void DocumentSessionMediaOpenWithRuntime::cancel()
{
    m_job.cancel();
    m_operation->cancel();
}

bool DocumentSessionMediaOpenWithRuntime::active() const { return m_operation->active(); }
}
