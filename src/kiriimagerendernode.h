// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGERENDERNODE_H
#define KIRIVIEW_KIRIIMAGERENDERNODE_H

#include <QImage>
#include <QRectF>
#include <QSGRenderNode>
#include <memory>

class QRhi;
class QRhiBuffer;
class QRhiGraphicsPipeline;
class QRhiRenderPassDescriptor;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QRhiTexture;
class QSize;
class QSizeF;

namespace KiriView {
QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize);

class KiriImageRenderNode final : public QSGRenderNode
{
public:
    KiriImageRenderNode() = default;
    ~KiriImageRenderNode() override;

    void setRhi(QRhi *rhi);
    void setImage(const QImage &image, quint64 revision);
    void setTargetRect(const QRectF &targetRect);

    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;
    void prepare() override;
    void render(const RenderState *state) override;
    void releaseResources() override;

private:
    static void releasePendingResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates);

    QRhiResourceUpdateBatch *ensureResourceUpdates(QRhiResourceUpdateBatch *&resourceUpdates);
    bool ensureVertexBuffer(QRhiResourceUpdateBatch *&resourceUpdates);
    bool ensureTexture(QRhiResourceUpdateBatch *&resourceUpdates);
    bool ensureUniformBuffer();
    bool ensureSampler();
    bool ensureShaderResourceBindings();
    bool ensurePipeline(QRhiRenderTarget *renderTarget);
    void updateUniformBuffer(const RenderState *state);

    QRhi *m_rhi = nullptr;
    QImage m_image;
    quint64 m_imageRevision = 0;
    quint64 m_uploadedImageRevision = 0;
    QRectF m_targetRect;
    bool m_textureDirty = true;
    QRhiRenderPassDescriptor *m_renderPassDescriptor = nullptr;
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
};
}

#endif
