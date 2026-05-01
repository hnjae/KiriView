// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagerendernode.h"

#include "shaders/kiriimageview_shaders.h"

#include <QByteArray>
#include <QMatrix4x4>
#include <QSize>
#include <QSizeF>
#include <algorithm>
#include <array>
#include <cstddef>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>

namespace {
constexpr quint32 uniformBufferSize = 96;

struct Vertex {
    float position[2];
    float texCoord[2];
};

constexpr std::array<Vertex, 6> quadVertices = { {
    { { 0.0F, 0.0F }, { 0.0F, 0.0F } },
    { { 1.0F, 0.0F }, { 1.0F, 0.0F } },
    { { 0.0F, 1.0F }, { 0.0F, 1.0F } },
    { { 0.0F, 1.0F }, { 0.0F, 1.0F } },
    { { 1.0F, 0.0F }, { 1.0F, 0.0F } },
    { { 1.0F, 1.0F }, { 1.0F, 1.0F } },
} };

QShader shaderFromData(const unsigned char *data, std::size_t size)
{
    return QShader::fromSerialized(QByteArray::fromRawData(
        reinterpret_cast<const char *>(data), static_cast<qsizetype>(size)));
}

const QShader &imageVertexShader()
{
    static const QShader shader = shaderFromData(
        KiriView::kiriImageViewVertexShader, KiriView::kiriImageViewVertexShaderSize);
    return shader;
}

const QShader &imageFragmentShader()
{
    static const QShader shader = shaderFromData(
        KiriView::kiriImageViewFragmentShader, KiriView::kiriImageViewFragmentShaderSize);
    return shader;
}
}

namespace KiriView {
QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize)
{
    if (imageSize.isEmpty() || boundsSize.isEmpty()) {
        return {};
    }

    const qreal scale = std::min(
        boundsSize.width() / imageSize.width(), boundsSize.height() / imageSize.height());
    const QSizeF targetSize(imageSize.width() * scale, imageSize.height() * scale);
    return QRectF((boundsSize.width() - targetSize.width()) / 2.0,
        (boundsSize.height() - targetSize.height()) / 2.0, targetSize.width(), targetSize.height());
}

KiriImageRenderNode::~KiriImageRenderNode() { releaseResources(); }

void KiriImageRenderNode::setRhi(QRhi *rhi)
{
    if (m_rhi == rhi) {
        return;
    }

    releaseResources();
    m_rhi = rhi;
    m_uploadedImageRevision = 0;
    m_textureDirty = true;
}

void KiriImageRenderNode::setImage(const QImage &image, quint64 revision)
{
    if (m_imageRevision == revision) {
        return;
    }

    m_image = image;
    m_imageRevision = revision;
    m_textureDirty = true;
}

void KiriImageRenderNode::setTargetRect(const QRectF &targetRect) { m_targetRect = targetRect; }

QSGRenderNode::StateFlags KiriImageRenderNode::changedStates() const
{
    return BlendState | ScissorState | ViewportState;
}

QSGRenderNode::RenderingFlags KiriImageRenderNode::flags() const
{
    return BoundedRectRendering | NoExternalRendering;
}

QRectF KiriImageRenderNode::rect() const { return m_targetRect; }

void KiriImageRenderNode::prepare()
{
    if (m_rhi == nullptr || m_image.isNull()) {
        return;
    }

    auto *cb = commandBuffer();
    auto *rt = renderTarget();
    if (cb == nullptr || rt == nullptr) {
        return;
    }

    QRhiResourceUpdateBatch *resourceUpdates = nullptr;

    if (!ensureVertexBuffer(resourceUpdates)) {
        releasePendingResourceUpdates(resourceUpdates);
        return;
    }
    if (!ensureTexture(resourceUpdates)) {
        releasePendingResourceUpdates(resourceUpdates);
        return;
    }
    if (!ensureUniformBuffer() || !ensureSampler() || !ensureShaderResourceBindings()
        || !ensurePipeline(rt)) {
        releasePendingResourceUpdates(resourceUpdates);
        return;
    }

    if (resourceUpdates != nullptr) {
        cb->resourceUpdate(resourceUpdates);
    }
}

void KiriImageRenderNode::render(const RenderState *state)
{
    if (m_rhi == nullptr || m_pipeline == nullptr || m_srb == nullptr || m_uniformBuffer == nullptr
        || m_vertexBuffer == nullptr || m_targetRect.isEmpty()) {
        return;
    }

    auto *cb = commandBuffer();
    auto *rt = renderTarget();
    if (cb == nullptr || rt == nullptr) {
        return;
    }

    updateUniformBuffer(state);

    const QSize pixelSize = rt->pixelSize();
    cb->setViewport(QRhiViewport(
        0.0F, 0.0F, static_cast<float>(pixelSize.width()), static_cast<float>(pixelSize.height())));
    if (state != nullptr && state->scissorEnabled()) {
        const QRect scissorRect = state->scissorRect();
        cb->setScissor(QRhiScissor(
            scissorRect.x(), scissorRect.y(), scissorRect.width(), scissorRect.height()));
    } else {
        cb->setScissor(QRhiScissor(0, 0, pixelSize.width(), pixelSize.height()));
    }

    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setShaderResources(m_srb.get());
    const QRhiCommandBuffer::VertexInput vertexBinding(m_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vertexBinding);
    cb->draw(static_cast<quint32>(quadVertices.size()));
}

void KiriImageRenderNode::releaseResources()
{
    m_pipeline.reset();
    m_srb.reset();
    m_sampler.reset();
    m_texture.reset();
    m_uniformBuffer.reset();
    m_vertexBuffer.reset();
    m_uploadedImageRevision = 0;
    m_textureDirty = true;
    m_renderPassDescriptor = nullptr;
}

void KiriImageRenderNode::releasePendingResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates)
{
    if (resourceUpdates != nullptr) {
        resourceUpdates->release();
    }
}

QRhiResourceUpdateBatch *KiriImageRenderNode::ensureResourceUpdates(
    QRhiResourceUpdateBatch *&resourceUpdates)
{
    if (resourceUpdates == nullptr) {
        resourceUpdates = m_rhi->nextResourceUpdateBatch();
    }
    return resourceUpdates;
}

bool KiriImageRenderNode::ensureVertexBuffer(QRhiResourceUpdateBatch *&resourceUpdates)
{
    if (m_vertexBuffer != nullptr) {
        return true;
    }

    m_vertexBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
        static_cast<quint32>(sizeof(Vertex) * quadVertices.size())));
    if (m_vertexBuffer == nullptr || !m_vertexBuffer->create()) {
        m_vertexBuffer.reset();
        return false;
    }

    ensureResourceUpdates(resourceUpdates)
        ->uploadStaticBuffer(m_vertexBuffer.get(), quadVertices.data());
    return true;
}

bool KiriImageRenderNode::ensureTexture(QRhiResourceUpdateBatch *&resourceUpdates)
{
    if (!m_textureDirty && m_uploadedImageRevision == m_imageRevision) {
        return true;
    }

    if (m_texture == nullptr || m_texture->pixelSize() != m_image.size()) {
        m_pipeline.reset();
        m_srb.reset();
        m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, m_image.size()));
        if (m_texture == nullptr || !m_texture->create()) {
            m_texture.reset();
            return false;
        }
    }

    ensureResourceUpdates(resourceUpdates)->uploadTexture(m_texture.get(), m_image);
    m_uploadedImageRevision = m_imageRevision;
    m_textureDirty = false;
    return true;
}

bool KiriImageRenderNode::ensureUniformBuffer()
{
    if (m_uniformBuffer != nullptr) {
        return true;
    }

    m_pipeline.reset();
    m_srb.reset();
    m_uniformBuffer.reset(
        m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, uniformBufferSize));
    if (m_uniformBuffer == nullptr || !m_uniformBuffer->create()) {
        m_uniformBuffer.reset();
        return false;
    }
    return true;
}

bool KiriImageRenderNode::ensureSampler()
{
    if (m_sampler != nullptr) {
        return true;
    }

    m_pipeline.reset();
    m_srb.reset();
    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    if (m_sampler == nullptr || !m_sampler->create()) {
        m_sampler.reset();
        return false;
    }
    return true;
}

bool KiriImageRenderNode::ensureShaderResourceBindings()
{
    if (m_srb != nullptr) {
        return true;
    }

    m_pipeline.reset();
    m_srb.reset(m_rhi->newShaderResourceBindings());
    if (m_srb == nullptr) {
        return false;
    }

    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_uniformBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage, m_texture.get(), m_sampler.get()),
    });
    if (!m_srb->create()) {
        m_srb.reset();
        return false;
    }
    return true;
}

bool KiriImageRenderNode::ensurePipeline(QRhiRenderTarget *renderTarget)
{
    auto *renderPassDescriptor = renderTarget->renderPassDescriptor();
    if (m_pipeline != nullptr && m_renderPassDescriptor == renderPassDescriptor) {
        return true;
    }

    m_pipeline.reset();
    m_renderPassDescriptor = renderPassDescriptor;

    const QShader &vertexShader = imageVertexShader();
    const QShader &fragmentShader = imageFragmentShader();
    if (!vertexShader.isValid() || !fragmentShader.isValid()) {
        return false;
    }

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(sizeof(Vertex)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)),
    });

    QRhiGraphicsPipeline::TargetBlend targetBlend;
    targetBlend.enable = true;
    targetBlend.srcColor = QRhiGraphicsPipeline::One;
    targetBlend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    targetBlend.srcAlpha = QRhiGraphicsPipeline::One;
    targetBlend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    m_pipeline.reset(m_rhi->newGraphicsPipeline());
    if (m_pipeline == nullptr) {
        return false;
    }
    m_pipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
    m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    m_pipeline->setShaderStages({
        QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
        QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
    });
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(renderPassDescriptor);
    m_pipeline->setSampleCount(renderTarget->sampleCount());
    m_pipeline->setTargetBlends({ targetBlend });
    if (!m_pipeline->create()) {
        m_pipeline.reset();
        return false;
    }
    return true;
}

void KiriImageRenderNode::updateUniformBuffer(const RenderState *state)
{
    QMatrix4x4 matrix;
    if (state != nullptr && state->projectionMatrix() != nullptr) {
        matrix = *state->projectionMatrix();
    }
    if (const QMatrix4x4 *nodeMatrix = this->matrix()) {
        matrix *= *nodeMatrix;
    }

    const float opacity = static_cast<float>(inheritedOpacity());
    std::array<float, uniformBufferSize / sizeof(float)> uniformData {};
    std::copy(matrix.constData(), matrix.constData() + 16, uniformData.begin());
    uniformData[16] = static_cast<float>(m_targetRect.x());
    uniformData[17] = static_cast<float>(m_targetRect.y());
    uniformData[18] = static_cast<float>(m_targetRect.width());
    uniformData[19] = static_cast<float>(m_targetRect.height());
    uniformData[20] = opacity;

    m_uniformBuffer->fullDynamicBufferUpdateForCurrentFrame(uniformData.data(), uniformBufferSize);
}
}
