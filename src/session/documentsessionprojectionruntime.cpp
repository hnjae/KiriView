// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionprojectionruntime.h"

#include <utility>

namespace kiriview {
DocumentSessionProjectionRuntime::DocumentSessionProjectionRuntime(
    DocumentSessionProjectionRuntimePorts ports)
    : m_ports(std::move(ports))
{
}

void DocumentSessionProjectionRuntime::publish(const DocumentSessionPublicSnapshotInput& input,
    const ImageDocumentPageNavigationSnapshot& imageDocumentPageNavigationRows)
{
    if (m_ports.updatePublicSnapshot) {
        m_ports.updatePublicSnapshot(input);
    }
    syncActiveNavigationThumbnailRows(imageDocumentPageNavigationRows);
    clearActiveNavigationRevealContextIfUnavailable();
}

void DocumentSessionProjectionRuntime::publishForSourceKind(
    const DocumentSessionPublicSnapshotInput& input, ActiveNavigationSourceKind sourceKind,
    const ImageDocumentPageNavigationSnapshot& imageDocumentPageNavigationRows)
{
    const bool updated = m_ports.updatePublicSnapshotForSourceKind
        && m_ports.updatePublicSnapshotForSourceKind(input, sourceKind);
    if (updated) {
        syncActiveNavigationThumbnailRows(imageDocumentPageNavigationRows);
    }
    clearActiveNavigationRevealContextIfUnavailable();
}

void DocumentSessionProjectionRuntime::syncActiveNavigationThumbnailRows(
    const ImageDocumentPageNavigationSnapshot& imageDocumentPageNavigationRows)
{
    const ActiveNavigationSourceKind sourceKind = m_ports.activeNavigationSourceKind
        ? m_ports.activeNavigationSourceKind()
        : ActiveNavigationSourceKind::None;
    const ActiveNavigationSnapshot navigation = m_ports.activeNavigationSnapshot
        ? m_ports.activeNavigationSnapshot()
        : ActiveNavigationSnapshot {};
    const std::vector<DirectMediaNavigationCandidate> directMediaNavigationCandidates
        = m_ports.directMediaNavigationCandidates ? m_ports.directMediaNavigationCandidates()
                                                  : std::vector<DirectMediaNavigationCandidate> {};
    std::vector<ActiveNavigationThumbnailRow> rows = projectActiveNavigationThumbnailRows(
        sourceKind, navigation, directMediaNavigationCandidates, imageDocumentPageNavigationRows);
    if (m_ports.setActiveNavigationThumbnailRows) {
        m_ports.setActiveNavigationThumbnailRows(std::move(rows));
    }
}

void DocumentSessionProjectionRuntime::clearActiveNavigationRevealContextIfUnavailable()
{
    if (m_ports.clearActiveNavigationRevealContextIfUnavailable) {
        m_ports.clearActiveNavigationRevealContextIfUnavailable();
    }
}
}
