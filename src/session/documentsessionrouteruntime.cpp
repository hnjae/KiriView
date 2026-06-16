// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteruntime.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace kiriview {
DocumentSessionRouteRuntime::DocumentSessionRouteRuntime(DocumentSessionRouteRuntimePorts ports)
    : m_ports(std::move(ports))
{
}

void DocumentSessionRouteRuntime::execute(const DocumentSessionRoutePlan &plan)
{
    struct RouteExecutionState {
        bool directMediaScopeChanged = false;
        bool directMediaNavigationCleared = false;
    };

    RouteExecutionState state;
    if (m_ports.cancelMediaOpenWith) {
        m_ports.cancelMediaOpenWith();
    }

    for (const DocumentSessionRouteOperation &operation : plan.operations) {
        std::visit(
            [this, &state](const auto &payload) {
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
                    state.directMediaNavigationCleared = true;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = (m_ports.clearDirectMediaCursor && m_ports.clearDirectMediaCursor())
                        || state.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         SetDirectVideoCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = (m_ports.setDirectVideoCursor
                              && m_ports.setDirectVideoCursor(payload.url))
                        || state.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         RequestDirectImageCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = (m_ports.requestDirectImageCursor
                              && m_ports.requestDirectImageCursor(payload.url))
                        || state.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearThenRequestDirectImageCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = (m_ports.clearDirectMediaCursor && m_ports.clearDirectMediaCursor())
                        || state.directMediaScopeChanged;
                    state.directMediaScopeChanged
                        = (m_ports.requestDirectImageCursor
                              && m_ports.requestDirectImageCursor(payload.url))
                        || state.directMediaScopeChanged;
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
                    state.directMediaScopeChanged
                        = (m_ports.syncDirectImageCursorFromDocument
                              && m_ports.syncDirectImageCursorFromDocument())
                        || state.directMediaScopeChanged;
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
                } else if constexpr (std::is_same_v<Operation,
                                         RecomputePublicProjectionRouteOperation>) {
                    if (m_ports.recomputePublicProjection) {
                        m_ports.recomputePublicProjection();
                    }
                } else if constexpr (std::is_same_v<Operation,
                                         RefreshDirectMediaNavigationAfterRoutingRouteOperation>) {
                    const bool directMediaNavigationActive = m_ports.directMediaNavigationActive
                        && m_ports.directMediaNavigationActive();
                    if (state.directMediaScopeChanged || state.directMediaNavigationCleared
                        || directMediaNavigationActive) {
                        if (m_ports.refreshDirectMediaNavigation) {
                            m_ports.refreshDirectMediaNavigation();
                        }
                    }
                } else if constexpr (std::is_same_v<Operation, ClearMediaPredecodeRouteOperation>) {
                    if (state.directMediaNavigationCleared) {
                        if (m_ports.clearDirectMediaNavigation) {
                            m_ports.clearDirectMediaNavigation();
                        }
                        if (m_ports.recomputePublicProjection) {
                            m_ports.recomputePublicProjection();
                        }
                    }
                    if (m_ports.clearMediaPredecode) {
                        m_ports.clearMediaPredecode();
                    }
                }
            },
            operation);
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
