// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedeletioncontroller.h"

#include "imagecallback.h"
#include "imageremovalfallback.h"
#include "imageremovalfallbackexecutor.h"
#include "imageviewtext.h"

#include <utility>

namespace {
QString genericFileDeletionErrorMessage()
{
    return KiriView::imageViewText("Could not delete the selected file.");
}
}

namespace KiriView {
ImageDeletionController::ImageDeletionController(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, FileOperationProvider fileOperationProvider,
    Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_fallbackExecutor(
          std::make_unique<ImageRemovalFallbackExecutor>(this, std::move(candidateProvider),
              [this](ImageDeletionEffect effect) { report(std::move(effect)); }))
    , m_fileOperationProvider(fileOperationProviderWithDefault(std::move(fileOperationProvider)))
{
}

ImageDeletionController::~ImageDeletionController() { cancel(); }

bool ImageDeletionController::inProgress() const { return m_inProgress; }

void ImageDeletionController::deleteDisplayedFile(
    const DisplayedImageLocation &location, FileDeletionMode mode)
{
    if (m_inProgress) {
        return;
    }

    const QUrl targetUrl = deletionTargetUrlForDisplayedLocation(location);
    if (targetUrl.isEmpty()) {
        return;
    }

    const ImageRemovalFallbackPlan fallbackPlan
        = imageRemovalFallbackPlanForDisplayedLocation(location);

    setInProgress(true);
    m_fileDeletionJob = m_fileOperationProvider(this, FileDeletionRequest { targetUrl, mode },
        [this, fallbackPlan](FileDeletionResult result, const QString &errorString) {
            finishFileDeletion(fallbackPlan, result, errorString);
        });
}

void ImageDeletionController::finishFileDeletion(const ImageRemovalFallbackPlan &fallbackPlan,
    FileDeletionResult result, const QString &errorString)
{
    setInProgress(false);

    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback:
        report(ImageDeletionEffect::clearDeletedImage());
        openRemovalFallback(fallbackPlan);
        return;
    case FileDeletionCompletionAction::Ignore:
        return;
    case FileDeletionCompletionAction::ReportFailure:
        reportFailure(errorString);
        return;
    }
}

void ImageDeletionController::openRemovalFallback(const ImageRemovalFallbackPlan &fallbackPlan)
{
    m_fallbackExecutor->open(fallbackPlan);
}

void ImageDeletionController::setInProgress(bool inProgress)
{
    if (m_inProgress == inProgress) {
        return;
    }

    m_inProgress = inProgress;
    invokeIfSet(m_callbacks.inProgressChanged);
}

void ImageDeletionController::cancel()
{
    cancelFileDeletion();
    cancelFallback();
}

void ImageDeletionController::cancelFileDeletion()
{
    m_fileDeletionJob.cancel();
    setInProgress(false);
}

void ImageDeletionController::cancelFallback() { m_fallbackExecutor->cancel(); }

void ImageDeletionController::report(ImageDeletionEffect effect)
{
    invokeIfSet(m_callbacks.effect, std::move(effect));
}

void ImageDeletionController::reportFailure(const QString &errorString)
{
    const QString message = errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString;
    report(ImageDeletionEffect::reportFailure(message));
}
}
