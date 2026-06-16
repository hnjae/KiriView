// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteruntime.h"

#include "navigation/navigationlogging.h"

#include <QDebug>
#include <type_traits>
#include <utility>
#include <variant>

namespace {
const char *documentKindName(kiriview::DocumentSessionKind kind)
{
    switch (kind) {
    case kiriview::DocumentSessionKind::Empty:
        return "Empty";
    case kiriview::DocumentSessionKind::Image:
        return "Image";
    case kiriview::DocumentSessionKind::Video:
        return "Video";
    }

    return "Unknown";
}

const char *routeKindName(kiriview::DocumentSessionRouteKind kind)
{
    switch (kind) {
    case kiriview::DocumentSessionRouteKind::Empty:
        return "Empty";
    case kiriview::DocumentSessionRouteKind::DirectVideo:
        return "DirectVideo";
    case kiriview::DocumentSessionRouteKind::DirectImage:
        return "DirectImage";
    case kiriview::DocumentSessionRouteKind::ImageDocument:
        return "ImageDocument";
    }

    return "Unknown";
}
}

namespace kiriview {
DocumentSessionRouteRuntime::DocumentSessionRouteRuntime(DocumentSessionRouteRuntimePorts ports)
    : m_ports(std::move(ports))
{
}

void DocumentSessionRouteRuntime::routeSourceUrl(
    const QUrl &sourceUrl, DocumentSessionKind currentKind)
{
    const DocumentSessionRoutePlan plan
        = documentSessionRoutePlanForSourceUrl(sourceUrl, currentKind);
    qCDebug(kiriviewNavigationLog)
        << "route source url"
        << "url" << sourceUrl << "currentKind" << documentKindName(currentKind) << "routeKind"
        << routeKindName(plan.kind) << "mutations" << plan.mutations.size() << "followUpEffects"
        << plan.followUpEffects.size();
    execute(plan);
}

void DocumentSessionRouteRuntime::routeMediaUrl(const QUrl &url, DocumentSessionKind currentKind)
{
    const DocumentSessionRoutePlan plan = documentSessionRoutePlanForMediaUrl(url, currentKind);
    qCDebug(kiriviewNavigationLog)
        << "route media url"
        << "url" << url << "currentKind" << documentKindName(currentKind) << "routeKind"
        << routeKindName(plan.kind) << "mutations" << plan.mutations.size() << "followUpEffects"
        << plan.followUpEffects.size();
    execute(plan);
}

void DocumentSessionRouteRuntime::execute(const DocumentSessionRoutePlan &plan)
{
    struct RouteExecutionResult {
        bool directMediaScopeChanged = false;
        bool directMediaNavigationCleared = false;
        bool publishPublicProjection = false;
        bool refreshDirectMediaNavigation = false;
        bool clearDirectMediaNavigationBeforePredecode = false;
        bool clearMediaPredecode = false;
    };

    RouteExecutionResult result;
    if (m_ports.cancelMediaOpenWith) {
        m_ports.cancelMediaOpenWith();
    }

    result.publishPublicProjection = plan.publishPublicProjection;

    for (const DocumentSessionRouteMutation &mutation : plan.mutations) {
        std::visit(
            [this, &result](const auto &payload) {
                using Operation = std::decay_t<decltype(payload)>;

                if constexpr (std::is_same_v<Operation, ClearSessionErrorStringRouteOperation>) {
                    if (m_ports.clearSessionErrorString) {
                        m_ports.clearSessionErrorString();
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         CancelDirectMediaNavigationRouteOperation>) {
                    if (m_ports.cancelDirectMediaNavigation) {
                        m_ports.cancelDirectMediaNavigation();
                    }
                } else if constexpr (std::is_same_v<Operation, CancelMediaDeletionRouteOperation>) {
                    if (m_ports.cancelMediaDeletion) {
                        m_ports.cancelMediaDeletion();
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaNavigationRouteOperation>) {
                    if (m_ports.clearDirectMediaNavigation) {
                        m_ports.clearDirectMediaNavigation();
                    }
                    result.directMediaNavigationCleared = true;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.clearDirectMediaCursor && m_ports.clearDirectMediaCursor())
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         SetDirectVideoCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.setDirectVideoCursor
                              && m_ports.setDirectVideoCursor(payload.url))
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         RequestDirectImageCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.requestDirectImageCursor
                              && m_ports.requestDirectImageCursor(payload.url))
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearThenRequestDirectImageCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.clearDirectMediaCursor && m_ports.clearDirectMediaCursor())
                        || result.directMediaScopeChanged;
                    result.directMediaScopeChanged
                        = (m_ports.requestDirectImageCursor
                              && m_ports.requestDirectImageCursor(payload.url))
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation, ClearImageDocumentRouteOperation>) {
                    executeSuppressed([this]() {
                        if (m_ports.clearImageDocument) {
                            m_ports.clearImageDocument();
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, LeaveVideoModeRouteOperation>) {
                    executeSuppressed([this]() {
                        if (m_ports.leaveVideoMode) {
                            m_ports.leaveVideoMode();
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, EnterEmptyDocumentRouteOperation>) {
                    executeSuppressed([this]() {
                        if (m_ports.enterEmptyDocument) {
                            m_ports.enterEmptyDocument();
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, EnterImageDocumentRouteOperation>) {
                    executeSuppressed([this, &payload]() {
                        if (m_ports.enterImageDocument) {
                            m_ports.enterImageDocument(payload.url);
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, EnterVideoDocumentRouteOperation>) {
                    executeSuppressed([this, &payload]() {
                        if (m_ports.enterVideoDocument) {
                            m_ports.enterVideoDocument(payload.url);
                        }
                    });
                } else if constexpr (std::is_same_v<Operation,
                                         SyncDirectImageCursorFromDocumentRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.syncDirectImageCursorFromDocument
                              && m_ports.syncDirectImageCursorFromDocument())
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation, ClearSourceIdentityRouteOperation>) {
                    if (m_ports.clearSourceIdentity) {
                        m_ports.clearSourceIdentity();
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         UseOriginalSourceIdentityRouteOperation>) {
                    if (m_ports.useOriginalSourceIdentity) {
                        m_ports.useOriginalSourceIdentity(payload.url);
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         UseImageDocumentSourceIdentityRouteOperation>) {
                    if (m_ports.useImageDocumentSourceIdentity) {
                        m_ports.useImageDocumentSourceIdentity();
                    }
                }
            },
            mutation);
    }

    for (const DocumentSessionRouteFollowUpEffect &effect : plan.followUpEffects) {
        std::visit(
            [this, &result](const auto &payload) {
                using Effect = std::decay_t<decltype(payload)>;

                if constexpr (std::is_same_v<Effect,
                                  RefreshDirectMediaNavigationAfterRoutingRouteEffect>) {
                    const bool directMediaNavigationActive = m_ports.directMediaNavigationActive
                        && m_ports.directMediaNavigationActive();
                    if (result.directMediaScopeChanged || result.directMediaNavigationCleared
                        || directMediaNavigationActive) {
                        result.refreshDirectMediaNavigation = true;
                    }
                } else if constexpr (std::is_same_v<Effect, ClearMediaPredecodeRouteEffect>) {
                    if (result.directMediaNavigationCleared) {
                        result.clearDirectMediaNavigationBeforePredecode = true;
                        result.publishPublicProjection = true;
                    }
                    result.clearMediaPredecode = true;
                }
            },
            effect);
    }

    if (result.clearDirectMediaNavigationBeforePredecode && m_ports.clearDirectMediaNavigation) {
        m_ports.clearDirectMediaNavigation();
    }
    if (result.publishPublicProjection && m_ports.recomputePublicProjection) {
        m_ports.recomputePublicProjection();
    }
    if (result.refreshDirectMediaNavigation && m_ports.refreshDirectMediaNavigation) {
        m_ports.refreshDirectMediaNavigation();
    }
    if (result.clearMediaPredecode && m_ports.clearMediaPredecode) {
        m_ports.clearMediaPredecode();
    }

    if (m_ports.routeCompleted) {
        m_ports.routeCompleted();
    }
}

void DocumentSessionRouteRuntime::executeSuppressed(const std::function<void()> &mutation)
{
    if (m_ports.executeWithRoutingSuppressed) {
        m_ports.executeWithRoutingSuppressed(mutation);
        return;
    }

    mutation();
}
}
