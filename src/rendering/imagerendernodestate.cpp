// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendernodestate.h"

#include "rendering/imagerotation.h"

#include <utility>

namespace KiriView {
const ImageSurfaceDrawContext &ImageRenderNodeState::drawContext() const { return m_drawContext; }

const QRectF &ImageRenderNodeState::targetRect() const { return m_drawContext.targetRect; }

int ImageRenderNodeState::rotationDegrees() const { return m_drawContext.rotationDegrees; }

void ImageRenderNodeState::setFrame(bool sameSurface, quint64 revision,
    quint64 renderContextGeneration, const ImageSurfaceDrawContext &context)
{
    setSurface(sameSurface, revision, renderContextGeneration);
    setDrawContext(context);
}

ImageRenderNodeSurfaceUpdate ImageRenderNodeState::setSurface(
    bool sameSurface, quint64 revision, quint64 renderContextGeneration)
{
    const bool revisionChanged = setSurfaceRevision(revision);
    const bool renderContextGenerationChanged = setRenderContextGeneration(renderContextGeneration);
    if (sameSurface && !revisionChanged && !renderContextGenerationChanged) {
        return {};
    }

    if (!revisionChanged && !renderContextGenerationChanged) {
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

bool ImageRenderNodeState::setRenderContextGeneration(quint64 renderContextGeneration)
{
    if (m_renderContextGeneration == renderContextGeneration) {
        return false;
    }

    m_renderContextGeneration = renderContextGeneration;
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
    if (m_drawContext.targetRect == targetRect) {
        return false;
    }

    m_drawContext.targetRect = targetRect;
    m_drawGeometryDirty = true;
    return true;
}

bool ImageRenderNodeState::setDrawContext(const ImageSurfaceDrawContext &context)
{
    ImageSurfaceDrawContext normalized = context;
    normalized.rotationDegrees = normalizedImageRotationDegrees(context.rotationDegrees);
    if (m_drawContext == normalized) {
        return false;
    }

    m_drawContext = normalized;
    m_drawGeometryDirty = true;
    return true;
}

bool ImageRenderNodeState::setRotationDegrees(int rotationDegrees)
{
    const int normalized = normalizedImageRotationDegrees(rotationDegrees);
    if (m_drawContext.rotationDegrees == normalized) {
        return false;
    }

    m_drawContext.rotationDegrees = normalized;
    m_drawGeometryDirty = true;
    return true;
}

bool ImageRenderNodeState::uploadedTexturesAreCurrent() const
{
    return !m_texturesDirty && m_uploadedSurfaceRevision == m_surfaceRevision
        && m_uploadedRenderContextGeneration == m_renderContextGeneration;
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

bool ImageRenderNodeState::drawEntryIdentitiesMatch(
    const std::vector<ImageSurfaceDrawIdentity> &identities) const
{
    return m_uploadedDrawIdentities == identities;
}

void ImageRenderNodeState::applyDrawGeometrySyncResult(bool synchronized)
{
    if (synchronized) {
        m_drawGeometryDirty = false;
        return;
    }

    m_texturesDirty = true;
}

void ImageRenderNodeState::markTexturesUploaded(std::vector<ImageSurfaceDrawIdentity> identities)
{
    m_uploadedSurfaceRevision = m_surfaceRevision;
    m_uploadedRenderContextGeneration = m_renderContextGeneration;
    m_uploadedDrawIdentities = std::move(identities);
    m_texturesDirty = false;
    m_drawGeometryDirty = false;
}

void ImageRenderNodeState::resetUploadedResources()
{
    m_uploadedSurfaceRevision = 0;
    m_uploadedRenderContextGeneration = 0;
    m_uploadedDrawIdentities.clear();
    m_texturesDirty = true;
    m_drawGeometryDirty = true;
}
}
