// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "localization/activenavigationboundarytext.h"

#include <KLocalizedString>

namespace {
QString firstBoundaryText(kiriview::ActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case kiriview::ActiveNavigationBoundaryScope::DirectMedia:
        return i18nc("@info:status", "First media item");
    case kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage:
        return i18nc("@info:status", "First image");
    case kiriview::ActiveNavigationBoundaryScope::None:
        return {};
    }

    return {};
}

QString lastBoundaryText(kiriview::ActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case kiriview::ActiveNavigationBoundaryScope::DirectMedia:
        return i18nc("@info:status", "Last media item");
    case kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage:
        return i18nc("@info:status", "Last image");
    case kiriview::ActiveNavigationBoundaryScope::None:
        return {};
    }

    return {};
}
}

namespace kiriview {
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

QString containerNavigationBoundaryFeedbackText(NavigationDirection direction)
{
    switch (direction) {
    case NavigationDirection::Previous:
        return i18nc("@info:status", "No previous collection");
    case NavigationDirection::Next:
        return i18nc("@info:status", "No next collection");
    }

    return {};
}
}
