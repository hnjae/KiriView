// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendernodestate.h"

#include "imagerotation.h"

namespace KiriView {
const QRectF &ImageRenderNodeState::targetRect() const { return m_targetRect; }

int ImageRenderNodeState::rotationDegrees() const { return m_rotationDegrees; }

ImageRenderNodeSurfaceUpdate ImageRenderNodeState::setSurface(bool sameSurface, quint64 revision)
{
    const bool revisionChanged = setSurfaceRevision(revision);
    if (sameSurface && !revisionChanged) {
        return {};
    }

    if (!revisionChanged) {
        markSurfaceChanged();
    }
    return ImageRenderNodeSurfaceUpdate { true };
}

bool ImageRenderNodeState::setSurfaceRevision(quint64 revision)
{
    if (m_surfaceRevision == revision) {
        return false;
    }

    m_surfaceRevision = revision;
    markSurfaceChanged();
    return true;
}

void ImageRenderNodeState::markSurfaceChanged()
{
    m_texturesDirty = true;
    m_drawGeometryDirty = true;
}

bool ImageRenderNodeState::setTargetRect(const QRectF &targetRect)
{
    if (m_targetRect == targetRect) {
        return false;
    }

    m_targetRect = targetRect;
    m_drawGeometryDirty = true;
    return true;
}

bool ImageRenderNodeState::setRotationDegrees(int rotationDegrees)
{
    const int normalized = normalizedImageRotationDegrees(rotationDegrees);
    if (m_rotationDegrees == normalized) {
        return false;
    }

    m_rotationDegrees = normalized;
    m_drawGeometryDirty = true;
    return true;
}

bool ImageRenderNodeState::uploadedTexturesAreCurrent() const
{
    return !m_texturesDirty && m_uploadedSurfaceRevision == m_surfaceRevision;
}

ImageRenderNodeTextureUpdatePlan ImageRenderNodeState::textureUpdatePlan() const
{
    if (!uploadedTexturesAreCurrent()) {
        return ImageRenderNodeTextureUpdatePlan::RebuildTextures;
    }
    if (m_drawGeometryDirty) {
        return ImageRenderNodeTextureUpdatePlan::SynchronizeDrawGeometry;
    }

    return ImageRenderNodeTextureUpdatePlan::ReuseTextures;
}

void ImageRenderNodeState::applyDrawGeometrySyncResult(bool synchronized)
{
    if (synchronized) {
        m_drawGeometryDirty = false;
        return;
    }

    m_texturesDirty = true;
}

void ImageRenderNodeState::markTexturesUploaded()
{
    m_uploadedSurfaceRevision = m_surfaceRevision;
    m_texturesDirty = false;
    m_drawGeometryDirty = false;
}

void ImageRenderNodeState::resetUploadedResources()
{
    m_uploadedSurfaceRevision = 0;
    m_texturesDirty = true;
    m_drawGeometryDirty = true;
}
}
