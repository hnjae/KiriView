// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagerendernode.h"

#include "shaders/kiriimageview_shaders.h"

#include <QByteArray>
#include <QMatrix4x4>
#include <QSize>
#include <algorithm>
#include <array>
#include <cstddef>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>
#include <utility>

namespace {
constexpr quint32 uniformBufferSize = 112;

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

class PendingResourceUpdates final
{
public:
    PendingResourceUpdates() = default;
    ~PendingResourceUpdates()
    {
        if (m_updates != nullptr) {
            m_updates->release();
        }
    }

    PendingResourceUpdates(const PendingResourceUpdates &) = delete;
    PendingResourceUpdates &operator=(const PendingResourceUpdates &) = delete;

    QRhiResourceUpdateBatch *&ref() { return m_updates; }
    QRhiResourceUpdateBatch *take() { return std::exchange(m_updates, nullptr); }

private:
    QRhiResourceUpdateBatch *m_updates = nullptr;
};

}

namespace KiriView {
KiriImageRenderNode::~KiriImageRenderNode() { releaseResources(); }

void KiriImageRenderNode::setRhi(QRhi *rhi)
{
    if (m_rhi == rhi) {
        return;
    }

    releaseResources();
    m_rhi = rhi;
    m_uploadedSurfaceRevision = 0;
    m_texturesDirty = true;
    m_drawGeometryDirty = true;
}

void KiriImageRenderNode::setSurface(
    std::shared_ptr<DisplayedImageSurface> surface, quint64 revision)
{
    if (m_surfaceRevision == revision && m_surface == surface) {
        return;
    }

    m_surface = std::move(surface);
    m_surfaceRevision = revision;
    m_texturesDirty = true;
    m_drawGeometryDirty = true;
}

void KiriImageRenderNode::setTargetRect(const QRectF &targetRect)
{
    if (m_targetRect == targetRect) {
        return;
    }

    m_targetRect = targetRect;
    m_drawGeometryDirty = true;
}

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
    if (m_rhi == nullptr || m_surface == nullptr || displayedImageSurfaceIsNull(*m_surface)) {
        return;
    }

    auto *cb = commandBuffer();
    auto *rt = renderTarget();
    if (cb == nullptr || rt == nullptr) {
        return;
    }

    PendingResourceUpdates resourceUpdates;

    if (!ensureVertexBuffer(resourceUpdates.ref())) {
        return;
    }
    if (!ensureSampler()) {
        return;
    }
    if (!ensureTextures(resourceUpdates.ref())) {
        return;
    }
    if (!ensurePipeline(rt)) {
        return;
    }

    if (QRhiResourceUpdateBatch *updates = resourceUpdates.take()) {
        cb->resourceUpdate(updates);
    }
}

void KiriImageRenderNode::render(const RenderState *state)
{
    if (m_rhi == nullptr || m_pipeline == nullptr || m_vertexBuffer == nullptr
        || m_targetRect.isEmpty() || m_drawTextures.empty()) {
        return;
    }

    auto *cb = commandBuffer();
    auto *rt = renderTarget();
    if (cb == nullptr || rt == nullptr) {
        return;
    }

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

    const QRhiCommandBuffer::VertexInput vertexBinding(m_vertexBuffer.get(), 0);
    cb->setGraphicsPipeline(m_pipeline.get());
    for (const DrawTexture &drawTexture : m_drawTextures) {
        if (drawTexture.uniformBuffer == nullptr || drawTexture.srb == nullptr
            || drawTexture.targetRect.isEmpty()) {
            continue;
        }

        updateUniformBuffer(drawTexture.uniformBuffer.get(), state, drawTexture.targetRect,
            drawTexture.textureRect);
        cb->setShaderResources(drawTexture.srb.get());
        cb->setVertexInput(0, 1, &vertexBinding);
        cb->draw(static_cast<quint32>(quadVertices.size()));
    }
}

void KiriImageRenderNode::releaseResources()
{
    m_pipeline.reset();
    m_sampler.reset();
    m_drawTextures.clear();
    m_vertexBuffer.reset();
    m_uploadedSurfaceRevision = 0;
    m_texturesDirty = true;
    m_drawGeometryDirty = true;
    m_renderPassDescriptor = nullptr;
}

QRhiResourceUpdateBatch *KiriImageRenderNode::ensureResourceUpdates(
    QRhiResourceUpdateBatch *&resourceUpdates)
{
    if (resourceUpdates == nullptr) {
        resourceUpdates = m_rhi->nextResourceUpdateBatch();
    }
    return resourceUpdates;
}

bool KiriImageRenderNode::addDrawTexture(
    QRhiResourceUpdateBatch *&resourceUpdates, const ImageSurfaceDrawEntry &entry)
{
    if (entry.image.isNull() || entry.targetRect.isEmpty()) {
        return true;
    }

    DrawTexture drawTexture;
    drawTexture.targetRect = entry.targetRect;
    drawTexture.textureRect = entry.textureRect;
    drawTexture.uniformBuffer.reset(
        m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, uniformBufferSize));
    if (drawTexture.uniformBuffer == nullptr || !drawTexture.uniformBuffer->create()) {
        return false;
    }
    drawTexture.texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, entry.image.size(), 1,
        QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips));
    if (drawTexture.texture == nullptr || !drawTexture.texture->create()) {
        return false;
    }

    drawTexture.srb.reset(m_rhi->newShaderResourceBindings());
    if (drawTexture.srb == nullptr) {
        return false;
    }
    drawTexture.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            drawTexture.uniformBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
            drawTexture.texture.get(), m_sampler.get()),
    });
    if (!drawTexture.srb->create()) {
        return false;
    }

    ensureResourceUpdates(resourceUpdates)->uploadTexture(drawTexture.texture.get(), entry.image);
    ensureResourceUpdates(resourceUpdates)->generateMips(drawTexture.texture.get());
    m_drawTextures.push_back(std::move(drawTexture));
    return true;
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

bool KiriImageRenderNode::ensureTextures(QRhiResourceUpdateBatch *&resourceUpdates)
{
    if (!m_texturesDirty && m_uploadedSurfaceRevision == m_surfaceRevision) {
        if (m_drawGeometryDirty && !syncDrawTextureEntries()) {
            m_texturesDirty = true;
        } else {
            return true;
        }
    }

    if (!m_texturesDirty && m_uploadedSurfaceRevision == m_surfaceRevision) {
        return true;
    }

    m_pipeline.reset();
    m_drawTextures.clear();
    const std::vector<ImageSurfaceDrawEntry> entries
        = imageSurfaceDrawEntries(*m_surface, m_targetRect);
    for (const ImageSurfaceDrawEntry &entry : entries) {
        if (!addDrawTexture(resourceUpdates, entry)) {
            m_drawTextures.clear();
            return false;
        }
    }

    m_uploadedSurfaceRevision = m_surfaceRevision;
    m_texturesDirty = false;
    m_drawGeometryDirty = false;
    return true;
}

bool KiriImageRenderNode::syncDrawTextureEntries()
{
    if (m_surface == nullptr) {
        return false;
    }

    const std::vector<ImageSurfaceDrawEntry> entries
        = imageSurfaceDrawEntries(*m_surface, m_targetRect);
    if (entries.size() != m_drawTextures.size()) {
        return false;
    }

    for (std::size_t index = 0; index < entries.size(); ++index) {
        m_drawTextures[index].targetRect = entries[index].targetRect;
        m_drawTextures[index].textureRect = entries[index].textureRect;
    }
    m_drawGeometryDirty = false;
    return true;
}

bool KiriImageRenderNode::ensureSampler()
{
    if (m_sampler != nullptr) {
        return true;
    }

    m_pipeline.reset();
    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    if (m_sampler == nullptr || !m_sampler->create()) {
        m_sampler.reset();
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
    if (m_drawTextures.empty() || m_drawTextures.front().srb == nullptr) {
        return false;
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
    m_pipeline->setShaderResourceBindings(m_drawTextures.front().srb.get());
    m_pipeline->setRenderPassDescriptor(renderPassDescriptor);
    m_pipeline->setSampleCount(renderTarget->sampleCount());
    m_pipeline->setTargetBlends({ targetBlend });
    if (!m_pipeline->create()) {
        m_pipeline.reset();
        return false;
    }
    return true;
}

void KiriImageRenderNode::updateUniformBuffer(QRhiBuffer *uniformBuffer, const RenderState *state,
    const QRectF &targetRect, const QRectF &textureRect)
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
    uniformData[16] = static_cast<float>(targetRect.x());
    uniformData[17] = static_cast<float>(targetRect.y());
    uniformData[18] = static_cast<float>(targetRect.width());
    uniformData[19] = static_cast<float>(targetRect.height());
    uniformData[20] = opacity;
    uniformData[24] = static_cast<float>(textureRect.x());
    uniformData[25] = static_cast<float>(textureRect.y());
    uniformData[26] = static_cast<float>(textureRect.width());
    uniformData[27] = static_cast<float>(textureRect.height());

    uniformBuffer->fullDynamicBufferUpdateForCurrentFrame(uniformData.data(), uniformBufferSize);
}
}
