// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGERENDERNODE_H
#define KIRIVIEW_KIRIIMAGERENDERNODE_H

#include "imagerendering.h"

#include <QRectF>
#include <QSGRenderNode>
#include <memory>
#include <vector>

class QRhi;
class QRhiBuffer;
class QRhiGraphicsPipeline;
class QRhiRenderPassDescriptor;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QRhiTexture;

namespace KiriView {
class KiriImageRenderNode final : public QSGRenderNode
{
public:
    KiriImageRenderNode() = default;
    ~KiriImageRenderNode() override;

    void setRhi(QRhi *rhi);
    void setSurface(std::shared_ptr<DisplayedImageSurface> surface, quint64 revision);
    void setTargetRect(const QRectF &targetRect);

    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;
    void prepare() override;
    void render(const RenderState *state) override;
    void releaseResources() override;

private:
    QRhiResourceUpdateBatch *ensureResourceUpdates(QRhiResourceUpdateBatch *&resourceUpdates);
    bool addDrawTexture(
        QRhiResourceUpdateBatch *&resourceUpdates, const ImageSurfaceDrawEntry &entry);
    bool ensureVertexBuffer(QRhiResourceUpdateBatch *&resourceUpdates);
    bool ensureTextures(QRhiResourceUpdateBatch *&resourceUpdates);
    bool uploadedTexturesAreCurrent() const;
    bool tryReuseUploadedTextures();
    bool rebuildDrawTextures(QRhiResourceUpdateBatch *&resourceUpdates);
    bool syncDrawTextureEntries();
    bool ensureSampler();
    bool ensurePipeline(QRhiRenderTarget *renderTarget);
    void updateUniformBuffer(QRhiBuffer *uniformBuffer, const RenderState *state,
        const QRectF &targetRect, const QRectF &textureRect);

    QRhi *m_rhi = nullptr;
    std::shared_ptr<DisplayedImageSurface> m_surface;
    quint64 m_surfaceRevision = 0;
    quint64 m_uploadedSurfaceRevision = 0;
    QRectF m_targetRect;
    bool m_texturesDirty = true;
    bool m_drawGeometryDirty = true;
    QRhiRenderPassDescriptor *m_renderPassDescriptor = nullptr;
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    struct DrawTexture {
        QRectF targetRect;
        QRectF textureRect;
        std::unique_ptr<QRhiBuffer> uniformBuffer;
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiShaderResourceBindings> srb;
    };
    std::vector<DrawTexture> m_drawTextures;
};
}

#endif
