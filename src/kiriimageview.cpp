// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include "kiriview/src/avifcompat.cxx.h"
#include "shaders/kiriimageview_shaders.h"

#include <KCoreDirLister>
#include <KFileItem>
#include <KIO/Job>
#include <KIO/StoredTransferJob>
#include <KJob>
#include <QBuffer>
#include <QByteArray>
#include <QCollator>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QImageReader>
#include <QLocale>
#include <QMatrix4x4>
#include <QQuickWindow>
#include <QRectF>
#include <QSGRenderNode>
#include <QStringList>
#include <QVector4D>
#include <Qt>
#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <optional>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <utility>

namespace {
constexpr int defaultAnimationFrameDelay = 100;
constexpr int minimumAnimationFrameDelay = 10;
constexpr const char *documentPortalHostPathAttribute = "user.document-portal.host-path";
constexpr quint32 uniformBufferSize = 96;

struct ImageNavigationCandidate {
    QUrl url;
    QString name;
};

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

int normalizedAnimationFrameDelay(int delay)
{
    if (delay < 0) {
        return defaultAnimationFrameDelay;
    }

    return std::max(delay, minimumAnimationFrameDelay);
}

bool isSupportedImageFileName(const QString &name)
{
    static const QStringList supportedExtensions = {
        QStringLiteral("avif"),
        QStringLiteral("bmp"),
        QStringLiteral("gif"),
        QStringLiteral("jpeg"),
        QStringLiteral("jpg"),
        QStringLiteral("png"),
        QStringLiteral("svg"),
        QStringLiteral("webp"),
    };

    const qsizetype dotIndex = name.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= 0 || dotIndex == name.size() - 1) {
        return false;
    }

    return supportedExtensions.contains(name.mid(dotIndex + 1).toCaseFolded());
}

bool isKioFuseArchiveScheme(const QString &scheme)
{
    static const QStringList archiveSchemes = {
        QStringLiteral("zip"),
    };

    return archiveSchemes.contains(scheme);
}

std::optional<QUrl> kioFuseArchiveUrl(const QString &localPath)
{
    const QString cleanPath = QDir::cleanPath(localPath);
    const QString runtimeDir = QFile::decodeName(qgetenv("XDG_RUNTIME_DIR"));
    const QString marker = QStringLiteral("/kio-fuse-");
    qsizetype markerIndex = -1;

    if (!runtimeDir.isEmpty()) {
        const QString prefix = QDir::cleanPath(runtimeDir) + marker;
        if (!cleanPath.startsWith(prefix)) {
            return std::nullopt;
        }
        markerIndex = prefix.size() - marker.size();
    } else {
        markerIndex = cleanPath.indexOf(marker);
        if (markerIndex < 0) {
            return std::nullopt;
        }
    }

    const qsizetype mountEnd = cleanPath.indexOf(QLatin1Char('/'), markerIndex + marker.size());
    if (mountEnd < 0 || mountEnd == cleanPath.size() - 1) {
        return std::nullopt;
    }

    const QString relativePath = cleanPath.mid(mountEnd + 1);
    const qsizetype schemeEnd = relativePath.indexOf(QLatin1Char('/'));
    if (schemeEnd <= 0 || schemeEnd == relativePath.size() - 1) {
        return std::nullopt;
    }

    const QString scheme = relativePath.left(schemeEnd);
    if (!isKioFuseArchiveScheme(scheme)) {
        return std::nullopt;
    }

    QUrl url;
    url.setScheme(scheme);
    url.setPath(relativePath.mid(schemeEnd));
    if (!url.isValid() || url.path().isEmpty()) {
        return std::nullopt;
    }

    return url;
}

QUrl navigationUrlForLocalPath(const QString &localPath)
{
    const std::optional<QUrl> kioUrl = kioFuseArchiveUrl(localPath);
    if (kioUrl.has_value()) {
        return kioUrl.value();
    }

    return QUrl::fromLocalFile(localPath);
}

std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items)
{
    std::vector<ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !isSupportedImageFileName(name)) {
            continue;
        }

        candidates.push_back(ImageNavigationCandidate { item.url(), name });
    }

    QCollator collator(QLocale::system());
    collator.setCaseSensitivity(Qt::CaseSensitive);
    collator.setNumericMode(false);
    collator.setIgnorePunctuation(false);

    std::stable_sort(candidates.begin(), candidates.end(),
        [&collator](const ImageNavigationCandidate &left, const ImageNavigationCandidate &right) {
            const int nameComparison = collator.compare(left.name, right.name);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }

            return left.url.toEncoded() < right.url.toEncoded();
        });

    return candidates;
}

std::optional<QUrl> documentPortalHostUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const QString localPath = url.toLocalFile();
    const QByteArray encodedLocalPath = QFile::encodeName(localPath);
    if (encodedLocalPath.isEmpty()) {
        return std::nullopt;
    }

    // File dialogs can return document-portal URLs; navigation needs the real directory.
    const ssize_t valueSize
        = getxattr(encodedLocalPath.constData(), documentPortalHostPathAttribute, nullptr, 0);
    if (valueSize <= 0) {
        return std::nullopt;
    }

    QByteArray value;
    value.resize(valueSize);

    const ssize_t bytesRead = getxattr(encodedLocalPath.constData(),
        documentPortalHostPathAttribute, value.data(), static_cast<std::size_t>(value.size()));
    if (bytesRead <= 0) {
        return std::nullopt;
    }

    value.resize(bytesRead);
    if (value.endsWith('\0')) {
        value.chop(1);
    }

    const QString hostPath = QFile::decodeName(value);
    if (hostPath.isEmpty() || hostPath == localPath) {
        return std::nullopt;
    }

    return navigationUrlForLocalPath(hostPath);
}

QUrl navigationSourceUrl(const QUrl &url)
{
    const std::optional<QUrl> hostUrl = documentPortalHostUrl(url);
    if (hostUrl.has_value()) {
        return hostUrl.value();
    }

    if (url.isLocalFile()) {
        const std::optional<QUrl> kioUrl = kioFuseArchiveUrl(url.toLocalFile());
        if (kioUrl.has_value()) {
            return kioUrl.value();
        }
    }

    return url;
}

QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize)
{
    if (imageSize.isEmpty() || boundsSize.isEmpty()) {
        return {};
    }

    const qreal scale = std::min<qreal>(1.0,
        std::min(boundsSize.width() / imageSize.width(), boundsSize.height() / imageSize.height()));
    const QSizeF targetSize(imageSize.width() * scale, imageSize.height() * scale);
    return QRectF((boundsSize.width() - targetSize.width()) / 2.0,
        (boundsSize.height() - targetSize.height()) / 2.0, targetSize.width(), targetSize.height());
}

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

class KiriImageRenderNode final : public QSGRenderNode
{
public:
    KiriImageRenderNode() = default;
    ~KiriImageRenderNode() override { releaseResources(); }

    void setRhi(QRhi *rhi)
    {
        if (m_rhi == rhi) {
            return;
        }

        releaseResources();
        m_rhi = rhi;
        m_uploadedImageRevision = 0;
        m_textureDirty = true;
    }

    void setImage(const QImage &image, quint64 revision)
    {
        if (m_imageRevision == revision) {
            return;
        }

        m_image = image;
        m_imageRevision = revision;
        m_textureDirty = true;
    }

    void setTargetRect(const QRectF &targetRect) { m_targetRect = targetRect; }

    StateFlags changedStates() const override { return BlendState | ScissorState | ViewportState; }

    RenderingFlags flags() const override { return BoundedRectRendering | NoExternalRendering; }

    QRectF rect() const override { return m_targetRect; }

    void prepare() override
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

    void render(const RenderState *state) override
    {
        if (m_rhi == nullptr || m_pipeline == nullptr || m_srb == nullptr
            || m_uniformBuffer == nullptr || m_vertexBuffer == nullptr || m_targetRect.isEmpty()) {
            return;
        }

        auto *cb = commandBuffer();
        auto *rt = renderTarget();
        if (cb == nullptr || rt == nullptr) {
            return;
        }

        updateUniformBuffer(state);

        const QSize pixelSize = rt->pixelSize();
        cb->setViewport(QRhiViewport(0.0F, 0.0F, static_cast<float>(pixelSize.width()),
            static_cast<float>(pixelSize.height())));
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

    void releaseResources() override
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

private:
    static void releasePendingResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates)
    {
        if (resourceUpdates != nullptr) {
            resourceUpdates->release();
        }
    }

    QRhiResourceUpdateBatch *ensureResourceUpdates(QRhiResourceUpdateBatch *&resourceUpdates)
    {
        if (resourceUpdates == nullptr) {
            resourceUpdates = m_rhi->nextResourceUpdateBatch();
        }
        return resourceUpdates;
    }

    bool ensureVertexBuffer(QRhiResourceUpdateBatch *&resourceUpdates)
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

    bool ensureTexture(QRhiResourceUpdateBatch *&resourceUpdates)
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

    bool ensureUniformBuffer()
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

    bool ensureSampler()
    {
        if (m_sampler != nullptr) {
            return true;
        }

        m_pipeline.reset();
        m_srb.reset();
        m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
            QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        if (m_sampler == nullptr || !m_sampler->create()) {
            m_sampler.reset();
            return false;
        }
        return true;
    }

    bool ensureShaderResourceBindings()
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

    bool ensurePipeline(QRhiRenderTarget *renderTarget)
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

    void updateUniformBuffer(const RenderState *state)
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

        m_uniformBuffer->fullDynamicBufferUpdateForCurrentFrame(
            uniformData.data(), uniformBufferSize);
    }

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

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    m_animationTimer.setSingleShot(true);
    connect(&m_animationTimer, &QTimer::timeout, this, &KiriImageView::advanceAnimationFrame);
}

KiriImageView::~KiriImageView()
{
    stopAnimation();
    cancelNavigation();
    cancelLoad();
}

QUrl KiriImageView::sourceUrl() const { return m_sourceUrl; }

void KiriImageView::setSourceUrl(const QUrl &sourceUrl)
{
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    cancelNavigation();
    m_sourceUrl = sourceUrl;
    Q_EMIT sourceUrlChanged();
    startLoad();
}

KiriImageView::Status KiriImageView::status() const { return m_status; }

QString KiriImageView::errorString() const { return m_errorString; }

QSize KiriImageView::imageSize() const { return m_imageSize; }

void KiriImageView::openPreviousImage() { openAdjacentImage(NavigationDirection::Previous); }

void KiriImageView::openNextImage() { openAdjacentImage(NavigationDirection::Next); }

QSGNode *KiriImageView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_image.isNull()) {
        delete oldNode;
        return nullptr;
    }

    const QSizeF boundsSize(width(), height());
    if (boundsSize.isEmpty()) {
        delete oldNode;
        return nullptr;
    }

    auto *node = static_cast<KiriImageRenderNode *>(oldNode);
    if (node == nullptr) {
        node = new KiriImageRenderNode;
    }

    node->setRhi(window() == nullptr ? nullptr : window()->rhi());
    node->setImage(m_image, m_imageRevision);
    node->setTargetRect(imageTargetRect(m_image.size(), boundsSize));
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    return node;
}

void KiriImageView::startLoad()
{
    cancelLoad();
    clearImage();
    setErrorString(QString());

    if (m_sourceUrl.isEmpty()) {
        setStatus(Status::Null);
        return;
    }

    setStatus(Status::Loading);

    const quint64 generation = ++m_loadGeneration;
    auto *job = KIO::storedGet(m_sourceUrl, KIO::NoReload, KIO::HideProgressInfo);
    m_job = job;

    connect(job, &KJob::result, this, [this, job, generation](KJob *finishedJob) {
        if (generation != m_loadGeneration || job != m_job) {
            return;
        }

        m_job = nullptr;

        if (finishedJob->error() != KJob::NoError) {
            clearImage();
            setErrorString(finishedJob->errorString());
            setStatus(Status::Error);
            return;
        }

        finishWithImageData(job->data());
    });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_job) {
            m_job = nullptr;
        }
    });
}

void KiriImageView::cancelLoad()
{
    if (m_job == nullptr) {
        return;
    }

    auto *job = m_job;
    m_job = nullptr;
    ++m_loadGeneration;
    job->kill(KJob::Quietly);
}

void KiriImageView::openAdjacentImage(NavigationDirection direction)
{
    if (m_sourceUrl.isEmpty()) {
        return;
    }

    const QUrl currentUrl = navigationSourceUrl(m_sourceUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    auto *lister = new KCoreDirLister(this);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);

    const quint64 generation = ++m_navigationGeneration;
    m_navigationLister = lister;

    connect(lister, &KCoreDirLister::completed, this,
        [this, lister, generation, direction, currentUrl]() {
            finishNavigation(lister, generation, direction, currentUrl);
        });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, generation](KIO::Job *) { finishNavigationWithError(lister, generation); });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishNavigationWithError(lister, generation);
    }
}

void KiriImageView::cancelNavigation()
{
    if (m_navigationLister == nullptr) {
        return;
    }

    auto *lister = m_navigationLister;
    m_navigationLister = nullptr;
    ++m_navigationGeneration;
    lister->stop();
    lister->deleteLater();
}

void KiriImageView::finishNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentUrl)
{
    if (generation != m_navigationGeneration || lister != m_navigationLister) {
        return;
    }

    const std::vector<ImageNavigationCandidate> candidates
        = imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));

    m_navigationLister = nullptr;
    lister->deleteLater();

    const auto current = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const ImageNavigationCandidate &candidate) {
            return candidate.url.matches(currentUrl, QUrl::NormalizePathSegments);
        });
    if (current == candidates.cend()) {
        return;
    }

    QUrl targetUrl;
    if (direction == NavigationDirection::Previous) {
        if (current == candidates.cbegin()) {
            return;
        }
        targetUrl = std::prev(current)->url;
    } else {
        const auto next = std::next(current);
        if (next == candidates.cend()) {
            return;
        }
        targetUrl = next->url;
    }

    setSourceUrl(targetUrl);
}

void KiriImageView::finishNavigationWithError(KCoreDirLister *lister, quint64 generation)
{
    if (generation != m_navigationGeneration || lister != m_navigationLister) {
        return;
    }

    m_navigationLister = nullptr;
    lister->deleteLater();
}

void KiriImageView::finishWithImageData(const QByteArray &data)
{
    stopAnimation();

    KiriView::ApngDecodeResult apngResult = KiriView::decodeApngAnimation(data);
    if (apngResult.status == KiriView::ApngDecodeStatus::Success) {
        setDisplayedImage(apngResult.animation.frames.front().image);
        setErrorString(QString());
        setStatus(Status::Ready);
        startDecodedAnimation(
            std::move(apngResult.animation.frames), apngResult.animation.loopCount);
        return;
    }
    if (apngResult.status == KiriView::ApngDecodeStatus::Error) {
        clearImage();
        setErrorString(apngResult.errorString);
        setStatus(Status::Error);
        return;
    }

    const QByteArray imageData = KiriView::avifDataWithCompatibilityFixes(data);

    QBuffer buffer;
    buffer.setData(imageData);

    if (!buffer.open(QIODevice::ReadOnly)) {
        clearImage();
        setErrorString(tr("Could not read the selected image data."));
        setStatus(Status::Error);
        return;
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const bool supportsAnimation = reader.supportsAnimation();

    QImage image = reader.read();
    if (image.isNull()) {
        clearImage();
        setErrorString(reader.errorString());
        setStatus(Status::Error);
        return;
    }

    const QByteArray format = reader.format();
    const int firstFrameDelay = reader.nextImageDelay();
    const int loopCount = reader.loopCount();
    const bool hasMoreFrames = reader.canRead();

    setDisplayedImage(image);
    setErrorString(QString());
    setStatus(Status::Ready);

    if (supportsAnimation && hasMoreFrames) {
        startAnimation(imageData, format, loopCount, firstFrameDelay);
    }
}

void KiriImageView::startAnimation(
    const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay)
{
    m_animationData = data;
    m_animationFormat = format;
    m_animationLoopCount = loopCount;
    m_completedAnimationLoops = 0;

    QString errorString;
    if (!resetAnimationReader(&errorString)) {
        finishWithAnimationError(errorString);
        return;
    }

    const QImage firstFrame = m_animationReader->read();
    if (firstFrame.isNull()) {
        finishWithAnimationError(m_animationReader->errorString());
        return;
    }

    if (m_animationReader->canRead()) {
        m_animationTimer.start(normalizedAnimationFrameDelay(firstFrameDelay));
    }
}

void KiriImageView::startDecodedAnimation(
    std::vector<KiriView::AnimationFrame> frames, int loopCount)
{
    m_decodedAnimationFrames = std::move(frames);
    m_decodedAnimationFrameIndex = 0;
    m_animationLoopCount = loopCount;
    m_completedAnimationLoops = 0;

    if (m_decodedAnimationFrames.size() > 1) {
        m_animationTimer.start(
            normalizedAnimationFrameDelay(m_decodedAnimationFrames.front().delay));
    }
}

void KiriImageView::advanceAnimationFrame()
{
    if (!m_decodedAnimationFrames.empty()) {
        advanceDecodedAnimationFrame();
        return;
    }

    if (m_animationReader == nullptr) {
        return;
    }

    if (!m_animationReader->canRead()) {
        if (!hasRemainingAnimationLoops()) {
            stopAnimation();
            return;
        }

        ++m_completedAnimationLoops;

        QString errorString;
        if (!resetAnimationReader(&errorString)) {
            finishWithAnimationError(errorString);
            return;
        }
    }

    QImage frame = m_animationReader->read();
    if (frame.isNull()) {
        finishWithAnimationError(m_animationReader->errorString());
        return;
    }

    const int delay = normalizedAnimationFrameDelay(m_animationReader->nextImageDelay());
    setDisplayedImage(frame);

    if (m_animationReader->canRead() || hasRemainingAnimationLoops()) {
        m_animationTimer.start(delay);
    } else {
        stopAnimation();
    }
}

void KiriImageView::advanceDecodedAnimationFrame()
{
    if (m_decodedAnimationFrames.empty()) {
        return;
    }

    if (m_decodedAnimationFrameIndex + 1 >= m_decodedAnimationFrames.size()) {
        if (!hasRemainingAnimationLoops()) {
            stopAnimation();
            return;
        }

        ++m_completedAnimationLoops;
        m_decodedAnimationFrameIndex = 0;
    } else {
        ++m_decodedAnimationFrameIndex;
    }

    const KiriView::AnimationFrame &frame
        = m_decodedAnimationFrames.at(m_decodedAnimationFrameIndex);
    setDisplayedImage(frame.image);

    if (m_decodedAnimationFrameIndex + 1 < m_decodedAnimationFrames.size()
        || hasRemainingAnimationLoops()) {
        m_animationTimer.start(normalizedAnimationFrameDelay(frame.delay));
    } else {
        stopAnimation();
    }
}

bool KiriImageView::resetAnimationReader(QString *errorString)
{
    m_animationReader.reset();
    m_animationBuffer.reset();

    auto buffer = std::make_unique<QBuffer>();
    buffer->setData(m_animationData);

    if (!buffer->open(QIODevice::ReadOnly)) {
        *errorString = tr("Could not read the selected image data.");
        return false;
    }

    auto reader = std::make_unique<QImageReader>(buffer.get(), m_animationFormat);
    reader->setAutoTransform(true);

    if (!reader->canRead()) {
        *errorString = reader->errorString();
        return false;
    }

    m_animationBuffer = std::move(buffer);
    m_animationReader = std::move(reader);
    return true;
}

bool KiriImageView::hasRemainingAnimationLoops() const
{
    return m_animationLoopCount < 0 || m_completedAnimationLoops < m_animationLoopCount;
}

void KiriImageView::stopAnimation()
{
    m_animationTimer.stop();
    m_animationReader.reset();
    m_animationBuffer.reset();
    m_animationData.clear();
    m_animationFormat.clear();
    m_decodedAnimationFrames.clear();
    m_decodedAnimationFrameIndex = 0;
    m_animationLoopCount = 0;
    m_completedAnimationLoops = 0;
}

void KiriImageView::finishWithAnimationError(const QString &errorString)
{
    clearImage();
    const QString message = errorString.isEmpty()
        ? tr("Could not decode the selected image animation.")
        : errorString;
    setErrorString(message);
    setStatus(Status::Error);
}

void KiriImageView::setDisplayedImage(const QImage &image)
{
    m_image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    ++m_imageRevision;
    setImageSize(m_image.size());
    update();
}

void KiriImageView::setStatus(Status status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    Q_EMIT statusChanged();
}

void KiriImageView::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }

    m_errorString = errorString;
    Q_EMIT errorStringChanged();
}

void KiriImageView::setImageSize(const QSize &imageSize)
{
    if (m_imageSize == imageSize) {
        return;
    }

    m_imageSize = imageSize;
    Q_EMIT imageSizeChanged();
}

void KiriImageView::clearImage()
{
    stopAnimation();

    if (!m_image.isNull()) {
        m_image = QImage();
        ++m_imageRevision;
        update();
    }

    setImageSize(QSize());
}
