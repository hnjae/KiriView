// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERNODESTATE_H
#define KIRIVIEW_IMAGERENDERNODESTATE_H

#include "imagerendering.h"
#include "imagesurfacedrawcontext.h"

#include <QRectF>
#include <QtGlobal>
#include <vector>

namespace KiriView {
enum class ImageRenderNodeTextureUpdatePlan {
    ReuseTextures,
    SynchronizeDrawGeometry,
    RebuildTextures,
};

struct ImageRenderNodeSurfaceUpdate {
    bool acceptSurface = false;
};

class ImageRenderNodeState final
{
public:
    const ImageSurfaceDrawContext &drawContext() const;
    const QRectF &targetRect() const;
    int rotationDegrees() const;

    ImageRenderNodeSurfaceUpdate setSurface(bool sameSurface, quint64 revision);
    bool setDrawContext(const ImageSurfaceDrawContext &context);
    bool setTargetRect(const QRectF &targetRect);
    bool setRotationDegrees(int rotationDegrees);

    ImageRenderNodeTextureUpdatePlan textureUpdatePlan() const;
    bool drawEntryIdentitiesMatch(const std::vector<ImageSurfaceDrawIdentity> &identities) const;
    void applyDrawGeometrySyncResult(bool synchronized);
    void markTexturesUploaded(std::vector<ImageSurfaceDrawIdentity> identities = {});
    void resetUploadedResources();

private:
    bool setSurfaceRevision(quint64 revision);
    void markSurfaceChanged();
    bool uploadedTexturesAreCurrent() const;

    quint64 m_surfaceRevision = 0;
    quint64 m_uploadedSurfaceRevision = 0;
    ImageSurfaceDrawContext m_drawContext;
    std::vector<ImageSurfaceDrawIdentity> m_uploadedDrawIdentities;
    bool m_texturesDirty = true;
    bool m_drawGeometryDirty = true;
};
}

#endif
