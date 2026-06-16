// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionactivenavigationruntime.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace kiriview {
DocumentSessionActiveNavigationRuntime::DocumentSessionActiveNavigationRuntime(
    DocumentSessionActiveNavigationRuntimePorts ports)
    : m_ports(std::move(ports))
{
}

ActiveNavigationDispatchOutcome DocumentSessionActiveNavigationRuntime::dispatch(
    ActiveNavigationSourceKind sourceKind, ActiveNavigationSnapshot snapshot,
    ActiveNavigationDispatchRequest request, ActiveNavigationRevealContext context)
{
    const ActiveNavigationDispatchPlan plan
        = activeNavigationDispatchPlan(sourceKind, snapshot, request);
    if (plan.shouldDispatch()) {
        setPendingRevealContext(context);
    } else {
        m_pendingRevealContext = {};
        setRevealContext({});
        if (m_ports.recomputePublicProjection) {
            m_ports.recomputePublicProjection();
        }
    }
    executeDispatchPlan(plan);
    return plan.outcome;
}

void DocumentSessionActiveNavigationRuntime::setPendingRevealContext(
    ActiveNavigationRevealContext context)
{
    m_pendingRevealContext = context;
    setRevealContext(context);
}

ActiveNavigationRevealContext DocumentSessionActiveNavigationRuntime::takePendingRevealContext(
    ActiveNavigationRevealIntent fallbackIntent)
{
    const ActiveNavigationRevealContext context
        = m_pendingRevealContext.intent == ActiveNavigationRevealIntent::None
        ? ActiveNavigationRevealContext { fallbackIntent }
        : m_pendingRevealContext;
    m_pendingRevealContext = {};
    return context;
}

void DocumentSessionActiveNavigationRuntime::setRevealContext(ActiveNavigationRevealContext context)
{
    if (m_ports.setRevealContext) {
        m_ports.setRevealContext(context);
    }
}

void DocumentSessionActiveNavigationRuntime::clearRevealContextIfUnavailable(
    const ActiveNavigationSnapshot &snapshot)
{
    if (!snapshot.available || !snapshot.known) {
        setRevealContext({});
    }
}

void DocumentSessionActiveNavigationRuntime::executeDispatchPlan(
    const ActiveNavigationDispatchPlan &plan)
{
    if (!plan.shouldDispatch()) {
        return;
    }

    std::visit(
        [this](const auto &operation) {
            using Operation = std::decay_t<decltype(operation)>;
            if constexpr (std::is_same_v<Operation, OpenPreviousDirectMediaNavigationOperation>) {
                if (m_ports.openPreviousDirectMedia) {
                    m_ports.openPreviousDirectMedia();
                }
            } else if constexpr (std::is_same_v<Operation,
                                     OpenNextDirectMediaNavigationOperation>) {
                if (m_ports.openNextDirectMedia) {
                    m_ports.openNextDirectMedia();
                }
            } else if constexpr (std::is_same_v<Operation,
                                     OpenDirectMediaNavigationAtNumberOperation>) {
                if (m_ports.openDirectMediaAtNumber) {
                    m_ports.openDirectMediaAtNumber(operation.number);
                }
            } else if constexpr (std::is_same_v<Operation,
                                     OpenPreviousImageDocumentPageOperation>) {
                if (m_ports.openPreviousImageDocumentPage) {
                    m_ports.openPreviousImageDocumentPage();
                }
            } else if constexpr (std::is_same_v<Operation, OpenNextImageDocumentPageOperation>) {
                if (m_ports.openNextImageDocumentPage) {
                    m_ports.openNextImageDocumentPage();
                }
            } else if constexpr (std::is_same_v<Operation,
                                     OpenImageDocumentPageAtNumberOperation>) {
                if (m_ports.openImageDocumentPageAtNumber) {
                    m_ports.openImageDocumentPageAtNumber(operation.number);
                }
            }
        },
        plan.operation);
}
}
