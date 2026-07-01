// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationruntimeplan.h"

#include "localization/imageerrortext.h"

#include <iterator>
#include <type_traits>
#include <variant>

namespace {
template <typename> inline constexpr bool alwaysFalse = false;

void appendNavigationEffectRuntimeOperation(kiriview::ImageDocumentRuntimePlan& plan,
    const kiriview::ImageDocumentPageNavigationEffect& effect)
{
    std::visit(
        [&plan](const auto& payload) {
            using Effect = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Effect, kiriview::OpenImageDocumentPageUrlEffect>) {
                plan.push_back(kiriview::LoadPageNavigationUrlOperation { payload.target, false });
            } else if constexpr (std::is_same_v<Effect,
                                     kiriview::OpenContainerImageDocumentPageNavigationEffect>) {
                plan.push_back(kiriview::LoadContainerImageOperation {
                    payload.target,
                    payload.containerUrl,
                });
            } else if constexpr (std::is_same_v<Effect,
                                     kiriview::ReportContainerNavigationErrorEffect>) {
                if (payload.error == kiriview::ContainerNavigationError::EmptyContainer) {
                    plan.push_back(
                        kiriview::FinishEmptyContainerNavigationOperation { payload.containerUrl });
                    return;
                }

                if (payload.error == kiriview::ContainerNavigationError::InvalidComicBookArchive) {
                    plan.push_back(kiriview::FinishContainerNavigationLoadWithErrorOperation {
                        payload.containerUrl,
                        kiriview::imageErrorText(kiriview::ImageErrorTextId::OpenComicBookArchive),
                    });
                    return;
                }

                plan.push_back(kiriview::FinishContainerNavigationLoadWithErrorOperation {
                    payload.containerUrl,
                    payload.errorString,
                });
            } else if constexpr (std::is_same_v<Effect,
                                     kiriview::ReportContainerNavigationBoundaryEffect>) {
                plan.push_back(kiriview::ReportContainerNavigationBoundaryOperation {
                    payload.direction,
                });
            } else if constexpr (std::is_same_v<Effect,
                                     kiriview::ReportContainerNavigationListErrorEffect>) {
                plan.push_back(kiriview::ReportContainerNavigationListFailureOperation {
                    payload.failure,
                });
            } else if constexpr (std::is_same_v<Effect,
                                     kiriview::ClearCurrentImageDocumentPageNavigationEffect>) {
                kiriview::ImageDocumentRuntimePlan clearPlan
                    = kiriview::imageDocumentClearDeletedImagePlan();
                plan.insert(plan.end(), std::make_move_iterator(clearPlan.begin()),
                    std::make_move_iterator(clearPlan.end()));
            } else {
                static_assert(alwaysFalse<Effect>, "Unhandled image navigation effect");
            }
        },
        effect);
}
}

namespace kiriview {
ImageDocumentRuntimePlan imageDocumentRuntimePlanForNavigationPlan(
    const ImageDocumentPageNavigationPlan& navigationPlan)
{
    ImageDocumentRuntimePlan runtimePlan;
    runtimePlan.reserve(navigationPlan.size());
    for (const ImageDocumentPageNavigationEffect& effect : navigationPlan) {
        appendNavigationEffectRuntimeOperation(runtimePlan, effect);
    }
    return runtimePlan;
}
}
