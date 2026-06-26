// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIOOPERATIONFAILURE_H
#define KIRIVIEW_KIOOPERATIONFAILURE_H

#include <QString>
#include <QUrl>
#include <optional>

namespace kiriview {
enum class KioOperationKind {
    Unknown,
    FileDeletion,
    MediaOpenWith,
};

struct KioOperationFailure
{
    KioOperationKind operationKind = KioOperationKind::Unknown;
    QUrl targetUrl;
    std::optional<int> rawErrorCode;
    bool canceled = false;
    QString userMessage;
    QString diagnosticDetail;
    bool retryable = false;
};

bool isKioOperationCanceledError(int errorCode);
KioOperationFailure kioOperationFailureFromKJob(
    KioOperationKind operationKind, const QUrl& targetUrl, int errorCode, const QString& errorText);
KioOperationFailure kioOperationValidationFailure(
    KioOperationKind operationKind, const QUrl& targetUrl, const QString& diagnosticDetail);
}

#endif
