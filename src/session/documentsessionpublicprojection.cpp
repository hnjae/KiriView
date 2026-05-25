// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionpublicprojection.h"

#include "session/windowtitleprojection.h"

namespace {
bool imageDocumentPagesArePresent(const KiriView::ImageDocumentActiveNavigationInput &input)
{
    return input.currentNumber > 0 || input.count > 0;
}

KiriView::ActiveNavigationSourceKind sourceKindForInput(
    const KiriView::DocumentSessionPublicProjectionInput &input)
{
    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Video:
        return KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia;
    case KiriView::DocumentSessionKind::Image:
        if (input.directImageLoadMayUseMediaScope) {
            return KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia;
        }
        if (imageDocumentPagesArePresent(input.imageDocumentNavigation)
            || input.imageSourceMayRepresentDocument) {
            return KiriView::ActiveNavigationSourceKind::ImageDocumentPages;
        }
        return KiriView::ActiveNavigationSourceKind::None;
    case KiriView::DocumentSessionKind::Empty:
        return KiriView::ActiveNavigationSourceKind::None;
    }

    return KiriView::ActiveNavigationSourceKind::None;
}

QSize directMediaSizeForInput(const KiriView::DocumentSessionPublicProjectionInput &input,
    KiriView::ActiveNavigationSourceKind sourceKind)
{
    if (sourceKind != KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia) {
        return {};
    }

    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        return input.imageDirectMediaSize;
    case KiriView::DocumentSessionKind::Video:
        return input.videoDirectMediaSize;
    case KiriView::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

QString windowTitleFileNameForInput(const KiriView::DocumentSessionPublicProjectionInput &input)
{
    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        return input.imageWindowTitleFileName;
    case KiriView::DocumentSessionKind::Video:
        return input.videoWindowTitleFileName;
    case KiriView::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}
}

namespace KiriView {
DocumentSessionPublicProjection projectDocumentSessionPublicState(
    DocumentSessionPublicProjectionInput input)
{
    DocumentSessionPublicProjection projection;
    projection.sourceKind = sourceKindForInput(input);
    projection.boundaryScope = activeNavigationBoundaryScopeForSource(projection.sourceKind);
    projection.activeNavigation = projectActiveNavigation(projection.sourceKind,
        input.mediaNavigation, input.imageDocumentNavigation, input.fileDeletionInProgress);
    projection.windowTitleSubject = projectWindowTitleSubject(WindowTitleSubjectInput {
        windowTitleFileNameForInput(input),
        projection.sourceKind,
        directMediaSizeForInput(input, projection.sourceKind),
        projection.activeNavigation,
    });
    return projection;
}
}
