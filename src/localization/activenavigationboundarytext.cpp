// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "localization/activenavigationboundarytext.h"

#include <KLocalizedString>

namespace {
QString firstBoundaryText(KiriView::ActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case KiriView::ActiveNavigationBoundaryScope::DirectMedia:
        return i18nc("@info:status", "First media item");
    case KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage:
        return i18nc("@info:status", "First image");
    case KiriView::ActiveNavigationBoundaryScope::None:
        return {};
    }

    return {};
}

QString lastBoundaryText(KiriView::ActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case KiriView::ActiveNavigationBoundaryScope::DirectMedia:
        return i18nc("@info:status", "Last media item");
    case KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage:
        return i18nc("@info:status", "Last image");
    case KiriView::ActiveNavigationBoundaryScope::None:
        return {};
    }

    return {};
}
}

namespace KiriView {
QString activeNavigationBoundaryFeedbackText(
    ActiveNavigationBoundaryScope scope, ActiveNavigationDispatchOutcome outcome)
{
    switch (outcome) {
    case ActiveNavigationDispatchOutcome::FirstBoundary:
        return firstBoundaryText(scope);
    case ActiveNavigationDispatchOutcome::LastBoundary:
        return lastBoundaryText(scope);
    case ActiveNavigationDispatchOutcome::NoOp:
    case ActiveNavigationDispatchOutcome::Dispatch:
        return {};
    }

    return {};
}
}
