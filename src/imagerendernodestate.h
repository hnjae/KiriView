// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERNODESTATE_H
#define KIRIVIEW_IMAGERENDERNODESTATE_H

#include <QRectF>
#include <QtGlobal>

namespace KiriView {
class ImageRenderNodeState final
{
public:
    const QRectF &targetRect() const;
    int rotationDegrees() const;

    bool setSurfaceRevision(quint64 revision);
    void markSurfaceChanged();
    bool setTargetRect(const QRectF &targetRect);
    bool setRotationDegrees(int rotationDegrees);

    bool uploadedTexturesAreCurrent() const;
    bool drawGeometryDirty() const;
    void markDrawGeometrySynchronized();
    void markTextureReuseFailed();
    void markTexturesUploaded();
    void resetUploadedResources();

private:
    quint64 m_surfaceRevision = 0;
    quint64 m_uploadedSurfaceRevision = 0;
    QRectF m_targetRect;
    int m_rotationDegrees = 0;
    bool m_texturesDirty = true;
    bool m_drawGeometryDirty = true;
};
}

#endif
