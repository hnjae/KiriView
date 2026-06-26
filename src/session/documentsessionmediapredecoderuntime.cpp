// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionmediapredecoderuntime.h"

#include "predecode/mediapredecodecoordinator.h"

#include <QObject>
#include <utility>

namespace kiriview {
DocumentSessionMediaPredecodeRuntime::DocumentSessionMediaPredecodeRuntime(
    QObject* owner, MediaPredecodeDependencyOverrides dependencies)
    : m_coordinator(std::make_unique<MediaPredecodeCoordinator>(
          owner, resolveMediaPredecodeDependencies(std::move(dependencies))))
{
}

DocumentSessionMediaPredecodeRuntime::~DocumentSessionMediaPredecodeRuntime() = default;

void DocumentSessionMediaPredecodeRuntime::schedule(const DocumentSessionMediaPredecodeInput& input,
    std::vector<DirectMediaNavigationCandidate> candidates)
{
    if (!input.directMediaNavigationActive || input.currentUrl.isEmpty()) {
        return;
    }

    m_coordinator->schedule(MediaPredecodeCoordinator::Context {
        input.currentUrl,
        std::move(candidates),
        displayedImages(input),
        input.firstDisplayDecodeContext,
    });
}

void DocumentSessionMediaPredecodeRuntime::cacheDisplayedImages(
    const DocumentSessionMediaPredecodeInput& input)
{
    if (!input.directMediaNavigationActive) {
        return;
    }

    std::vector<DisplayedPredecodeImage> images = displayedImages(input);
    if (images.empty()) {
        return;
    }

    m_coordinator->cacheDisplayedImages(images);
}

void DocumentSessionMediaPredecodeRuntime::cancel() { m_coordinator->cancel(); }

void DocumentSessionMediaPredecodeRuntime::clear() { m_coordinator->clear(); }

std::optional<PredecodedImage> DocumentSessionMediaPredecodeRuntime::findPredecodedImage(
    const QUrl& url) const
{
    return m_coordinator->findPredecodedImage(url);
}

std::vector<DisplayedPredecodeImage> DocumentSessionMediaPredecodeRuntime::displayedImages(
    const DocumentSessionMediaPredecodeInput& input)
{
    if (input.documentKind != DocumentSessionKind::Image
        || !input.activeImageUsesImageDocumentSourceScope || !input.imageReady
        || !input.primaryDisplayedPredecodeImage.has_value()) {
        return {};
    }

    return { *input.primaryDisplayedPredecodeImage };
}
}
