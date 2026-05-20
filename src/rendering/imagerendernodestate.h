// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERNODESTATE_H
#define KIRIVIEW_IMAGERENDERNODESTATE_H

#include <QRectF>
#include <QtGlobal>

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
    const QRectF &targetRect() const;
    int rotationDegrees() const;

    ImageRenderNodeSurfaceUpdate setSurface(bool sameSurface, quint64 revision);
    bool setTargetRect(const QRectF &targetRect);
    bool setRotationDegrees(int rotationDegrees);

    ImageRenderNodeTextureUpdatePlan textureUpdatePlan() const;
    void applyDrawGeometrySyncResult(bool synchronized);
    void markTexturesUploaded();
    void resetUploadedResources();

private:
    bool setSurfaceRevision(quint64 revision);
    void markSurfaceChanged();
    bool uploadedTexturesAreCurrent() const;

    quint64 m_surfaceRevision = 0;
    quint64 m_uploadedSurfaceRevision = 0;
    QRectF m_targetRect;
    int m_rotationDegrees = 0;
    bool m_texturesDirty = true;
    bool m_drawGeometryDirty = true;
};
}

#endif
