// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/kiooperationfailure.h"

#include <KIO/Global>
#include <KJob>

namespace kiriview {
bool isKioOperationCanceledError(int errorCode)
{
    // Qt currently exposes these cancellation names with the same value.
    // NOLINTNEXTLINE(misc-redundant-expression)
    return errorCode == KJob::KilledJobError || errorCode == KIO::ERR_USER_CANCELED;
}

KioOperationFailure kioOperationFailureFromKJob(
    KioOperationKind operationKind, const QUrl& targetUrl, int errorCode, const QString& errorText)
{
    const bool canceled = isKioOperationCanceledError(errorCode);
    return KioOperationFailure {
        operationKind,
        targetUrl,
        errorCode,
        canceled,
        canceled ? QString() : errorText,
        errorText,
        !canceled,
    };
}

KioOperationFailure kioOperationValidationFailure(
    KioOperationKind operationKind, const QUrl& targetUrl, const QString& diagnosticDetail)
{
    return KioOperationFailure {
        operationKind,
        targetUrl,
        std::nullopt,
        false,
        QString(),
        diagnosticDetail,
        false,
    };
}
}
