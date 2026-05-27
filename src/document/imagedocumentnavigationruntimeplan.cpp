// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationruntimeplan.h"

#include "localization/imageerrortext.h"

#include <iterator>
#include <type_traits>
#include <variant>

namespace {
template <typename> inline constexpr bool alwaysFalse = false;

void appendNavigationEffectRuntimeOperation(
    KiriView::ImageDocumentRuntimePlan &plan, const KiriView::ImageNavigationEffect &effect)
{
    std::visit(
        [&plan](const auto &payload) {
            using Effect = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Effect, KiriView::OpenImageNavigationUrlEffect>) {
                plan.push_back(KiriView::LoadUrlOperation { payload.target });
            } else if constexpr (std::is_same_v<Effect,
                                     KiriView::OpenContainerImageNavigationEffect>) {
                plan.push_back(KiriView::LoadContainerImageOperation {
                    payload.target,
                    payload.containerUrl,
                });
            } else if constexpr (std::is_same_v<Effect,
                                     KiriView::ReportContainerNavigationErrorEffect>) {
                if (payload.error == KiriView::ContainerNavigationError::EmptyContainer) {
                    plan.push_back(
                        KiriView::FinishEmptyContainerNavigationOperation { payload.containerUrl });
                    return;
                }

                if (payload.error == KiriView::ContainerNavigationError::InvalidComicBookArchive) {
                    plan.push_back(KiriView::FinishContainerNavigationLoadWithErrorOperation {
                        payload.containerUrl,
                        KiriView::imageErrorText(KiriView::ImageErrorTextId::OpenComicBookArchive),
                    });
                    return;
                }

                plan.push_back(KiriView::FinishContainerNavigationLoadWithErrorOperation {
                    payload.containerUrl,
                    payload.errorString,
                });
            } else if constexpr (std::is_same_v<Effect,
                                     KiriView::ClearCurrentImageNavigationEffect>) {
                KiriView::ImageDocumentRuntimePlan clearPlan
                    = KiriView::imageDocumentClearDeletedImagePlan();
                plan.insert(plan.end(), std::make_move_iterator(clearPlan.begin()),
                    std::make_move_iterator(clearPlan.end()));
            } else {
                static_assert(alwaysFalse<Effect>, "Unhandled image navigation effect");
            }
        },
        effect);
}
}

namespace KiriView {
ImageDocumentRuntimePlan imageDocumentRuntimePlanForNavigationPlan(
    const ImageNavigationPlan &navigationPlan)
{
    ImageDocumentRuntimePlan runtimePlan;
    runtimePlan.reserve(navigationPlan.size());
    for (const ImageNavigationEffect &effect : navigationPlan) {
        appendNavigationEffectRuntimeOperation(runtimePlan, effect);
    }
    return runtimePlan;
}
}
