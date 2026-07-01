// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteruntime.h"

#include "navigation/navigationlogging.h"

#include <QDebug>
#include <type_traits>
#include <utility>
#include <variant>

namespace {
const char* documentKindName(kiriview::DocumentSessionKind kind)
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

const char* routeKindName(kiriview::DocumentSessionRouteKind kind)
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
    const QUrl& sourceUrl, DocumentSessionKind currentKind)
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

void DocumentSessionRouteRuntime::routeMediaUrl(const QUrl& url, DocumentSessionKind currentKind)
{
    const DocumentSessionRoutePlan plan = documentSessionRoutePlanForMediaUrl(url, currentKind);
    qCDebug(kiriviewNavigationLog)
        << "route media url"
        << "url" << url << "currentKind" << documentKindName(currentKind) << "routeKind"
        << routeKindName(plan.kind) << "mutations" << plan.mutations.size() << "followUpEffects"
        << plan.followUpEffects.size();
    execute(plan);
}

void DocumentSessionRouteRuntime::execute(const DocumentSessionRoutePlan& plan)
{
    struct RouteExecutionResult
    {
        bool directMediaScopeChanged = false;
        bool directMediaNavigationCleared = false;
        bool publishPublicProjection = false;
        bool refreshDirectMediaNavigation = false;
        bool clearDirectMediaNavigationBeforePredecode = false;
        bool clearMediaPredecode = false;
    };

    RouteExecutionResult result;
    if (m_ports.session.cancelMediaOpenWith) {
        m_ports.session.cancelMediaOpenWith();
    }

    result.publishPublicProjection = plan.publishPublicProjection;

    for (const DocumentSessionRouteMutation& mutation : plan.mutations) {
        std::visit(
            [this, &result](const auto& payload) {
                using Operation = std::decay_t<decltype(payload)>;

                if constexpr (std::is_same_v<Operation, ClearSessionErrorStringRouteOperation>) {
                    if (m_ports.session.clearSessionErrorString) {
                        m_ports.session.clearSessionErrorString();
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         CancelDirectMediaNavigationRouteOperation>) {
                    if (m_ports.directMedia.cancelDirectMediaNavigation) {
                        m_ports.directMedia.cancelDirectMediaNavigation();
                    }
                } else if constexpr (std::is_same_v<Operation, CancelMediaDeletionRouteOperation>) {
                    if (m_ports.directMedia.cancelMediaDeletion) {
                        m_ports.directMedia.cancelMediaDeletion();
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaNavigationRouteOperation>) {
                    if (m_ports.directMedia.clearDirectMediaNavigation) {
                        m_ports.directMedia.clearDirectMediaNavigation();
                    }
                    result.directMediaNavigationCleared = true;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.directMedia.clearDirectMediaCursor
                              && m_ports.directMedia.clearDirectMediaCursor())
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         SetDirectVideoCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.directMedia.setDirectVideoCursor
                              && m_ports.directMedia.setDirectVideoCursor(payload.url))
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         RequestDirectImageCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.directMedia.requestDirectImageCursor
                              && m_ports.directMedia.requestDirectImageCursor(payload.url))
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearThenRequestDirectImageCursorRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.directMedia.clearDirectMediaCursor
                              && m_ports.directMedia.clearDirectMediaCursor())
                        || result.directMediaScopeChanged;
                    result.directMediaScopeChanged
                        = (m_ports.directMedia.requestDirectImageCursor
                              && m_ports.directMedia.requestDirectImageCursor(payload.url))
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation, ClearImageDocumentRouteOperation>) {
                    executeSuppressed([this]() {
                        if (m_ports.documents.clearImageDocument) {
                            m_ports.documents.clearImageDocument();
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, LeaveVideoModeRouteOperation>) {
                    executeSuppressed([this]() {
                        if (m_ports.documents.leaveVideoMode) {
                            m_ports.documents.leaveVideoMode();
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, EnterEmptyDocumentRouteOperation>) {
                    executeSuppressed([this]() {
                        if (m_ports.documents.enterEmptyDocument) {
                            m_ports.documents.enterEmptyDocument();
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, EnterImageDocumentRouteOperation>) {
                    executeSuppressed([this, &payload]() {
                        if (m_ports.documents.enterImageDocument) {
                            m_ports.documents.enterImageDocument(payload.url);
                        }
                    });
                } else if constexpr (std::is_same_v<Operation,
                                         EnterImageDocumentSameScopeNavigationRouteOperation>) {
                    executeSuppressed([this, &payload]() {
                        if (m_ports.documents.enterImageDocumentSameScopeNavigation) {
                            m_ports.documents.enterImageDocumentSameScopeNavigation(payload.url);
                        }
                    });
                } else if constexpr (std::is_same_v<Operation, EnterVideoDocumentRouteOperation>) {
                    executeSuppressed([this, &payload]() {
                        if (m_ports.documents.enterVideoDocument) {
                            m_ports.documents.enterVideoDocument(payload.url);
                        }
                    });
                } else if constexpr (std::is_same_v<Operation,
                                         SyncDirectImageCursorFromDocumentRouteOperation>) {
                    result.directMediaScopeChanged
                        = (m_ports.directMedia.syncDirectImageCursorFromDocument
                              && m_ports.directMedia.syncDirectImageCursorFromDocument())
                        || result.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation, ClearSourceIdentityRouteOperation>) {
                    if (m_ports.sourceIdentity.clearSourceIdentity) {
                        m_ports.sourceIdentity.clearSourceIdentity();
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         UseOriginalSourceIdentityRouteOperation>) {
                    if (m_ports.sourceIdentity.useOriginalSourceIdentity) {
                        m_ports.sourceIdentity.useOriginalSourceIdentity(payload.url);
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         UseImageDocumentSourceIdentityRouteOperation>) {
                    if (m_ports.sourceIdentity.useImageDocumentSourceIdentity) {
                        m_ports.sourceIdentity.useImageDocumentSourceIdentity();
                    }
                }
            },
            mutation);
    }

    for (const DocumentSessionRouteFollowUpEffect& effect : plan.followUpEffects) {
        std::visit(
            [this, &result](const auto& payload) {
                using Effect = std::decay_t<decltype(payload)>;

                if constexpr (std::is_same_v<Effect,
                                  RefreshDirectMediaNavigationAfterRoutingRouteEffect>) {
                    const bool directMediaNavigationActive
                        = m_ports.directMedia.directMediaNavigationActive
                        && m_ports.directMedia.directMediaNavigationActive();
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

    if (result.clearDirectMediaNavigationBeforePredecode
        && m_ports.directMedia.clearDirectMediaNavigation) {
        m_ports.directMedia.clearDirectMediaNavigation();
    }
    if (result.publishPublicProjection && m_ports.followUp.recomputePublicProjection) {
        m_ports.followUp.recomputePublicProjection();
    }
    if (result.refreshDirectMediaNavigation && m_ports.directMedia.refreshDirectMediaNavigation) {
        m_ports.directMedia.refreshDirectMediaNavigation();
    }
    if (result.clearMediaPredecode && m_ports.followUp.clearMediaPredecode) {
        m_ports.followUp.clearMediaPredecode();
    }

    if (m_ports.session.routeCompleted) {
        m_ports.session.routeCompleted();
    }
}

void DocumentSessionRouteRuntime::executeSuppressed(const std::function<void()>& mutation)
{
    if (m_ports.session.executeWithRoutingSuppressed) {
        m_ports.session.executeWithRoutingSuppressed(mutation);
        return;
    }

    mutation();
}
}
