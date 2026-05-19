// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include "imagerendering.h"
#include "imageviewportgeometry.h"
#include "kiriimagedocument.h"
#include "kiriimagerendernode.h"

#include <QQuickWindow>
#include <cmath>
#include <memory>
#include <rhi/qrhi.h>

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

KiriImageView::~KiriImageView() { disconnectDocument(); }

KiriImageDocument *KiriImageView::document() const { return m_document; }

void KiriImageView::setDocument(KiriImageDocument *document)
{
    if (m_document == document) {
        return;
    }

    disconnectDocument();
    m_document = document;
    connectDocument();
    Q_EMIT documentChanged();
    update();
}

bool KiriImageView::secondaryPage() const { return m_secondaryPage; }

void KiriImageView::setSecondaryPage(bool secondaryPage)
{
    if (m_secondaryPage == secondaryPage) {
        return;
    }

    m_secondaryPage = secondaryPage;
    applyRenderContextBinding(m_renderContextBinding.setSecondaryPage(secondaryPage), m_document);
    Q_EMIT secondaryPageChanged();
    update();
}

QPointF KiriImageView::panContentPosition(
    const QPointF &contentPosition, const QPointF &delta) const
{
    return KiriView::imageViewportPanPosition(
        viewportSize(), viewportImageRect(), contentPosition, delta);
}

QPointF KiriImageView::nextScanContentPosition(const QPointF &contentPosition) const
{
    return KiriView::imageViewportNextZScanPosition(
        viewportSize(), viewportImageRect(), contentPosition, rightToLeftReadingActive());
}

QPointF KiriImageView::previousScanContentPosition(const QPointF &contentPosition) const
{
    return KiriView::imageViewportPreviousZScanPosition(
        viewportSize(), viewportImageRect(), contentPosition, rightToLeftReadingActive());
}

QPointF KiriImageView::initialScanContentPosition() const
{
    return KiriView::imageViewportInitialZScanPosition(
        viewportSize(), viewportImageRect(), rightToLeftReadingActive());
}

QPointF KiriImageView::finalScanContentPosition() const
{
    return KiriView::imageViewportFinalZScanPosition(
        viewportSize(), viewportImageRect(), rightToLeftReadingActive());
}

void KiriImageView::setNextDisplayedImageStartToFinalScanPosition()
{
    m_scanState.requestNextDisplayedImageFinalScanStart();
}

QPointF KiriImageView::displayedImageInitialContentPosition() const
{
    if (m_scanState.displayedImageScanStart() == KiriView::ImageViewportScanStart::Final) {
        return finalScanContentPosition();
    }

    return initialScanContentPosition();
}

bool KiriImageView::viewportPointInsideImage(
    const QPointF &contentPosition, const QPointF &viewportPoint) const
{
    return KiriView::imageViewportPointInsideImage(
        contentPosition, viewportPoint, viewportImageRect());
}

QPointF KiriImageView::zoomContentPosition(const QPointF &contentPosition,
    const QPointF &viewportAnchorPoint, double nextZoomPercent) const
{
    const QSizeF nextDisplaySize = KiriView::imageViewportDisplaySizeForZoom(
        imageSize(), nextZoomPercent, displayDevicePixelRatio());
    return KiriView::imageViewportContentPositionForZoom(
        viewportSize(), displaySize(), nextDisplaySize, contentPosition, viewportAnchorPoint);
}

QSGNode *KiriImageView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_document == nullptr) {
        delete oldNode;
        return nullptr;
    }

    KiriView::DisplayedImageRenderSnapshot snapshot = renderSnapshot();
    if (!snapshot.isRenderable()) {
        delete oldNode;
        return nullptr;
    }

    const QSizeF boundsSize(width(), height());
    if (boundsSize.isEmpty()) {
        delete oldNode;
        return nullptr;
    }

    auto *node = static_cast<KiriView::KiriImageRenderNode *>(oldNode);
    if (node == nullptr) {
        node = new KiriView::KiriImageRenderNode;
    }

    node->setRhi(window() == nullptr ? nullptr : window()->rhi());
    node->setSurface(std::move(snapshot.surface), snapshot.revision);
    node->setTargetRect(KiriView::imageTargetRect(snapshot.imageSize, boundsSize));
    node->setRotationDegrees(snapshot.rotationDegrees);
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    return node;
}

void KiriImageView::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);

    if (m_document != nullptr
        && (change == ItemSceneChange || change == ItemDevicePixelRatioHasChanged)) {
        m_document->updateRenderContext();
    }
}

void KiriImageView::classBegin()
{
    QQuickItem::classBegin();
    applyRenderContextBinding(m_renderContextBinding.setComponentComplete(false), m_document);
}

void KiriImageView::componentComplete()
{
    QQuickItem::componentComplete();
    applyRenderContextBinding(m_renderContextBinding.setComponentComplete(true), m_document);
}

KiriView::DisplayedPageRole KiriImageView::displayedPageRole() const
{
    return m_secondaryPage ? KiriView::DisplayedPageRole::Secondary
                           : KiriView::DisplayedPageRole::Primary;
}

QSize KiriImageView::imageSize() const
{
    return m_document == nullptr ? QSize() : m_document->imageSize();
}

KiriView::DisplayedImageRenderSnapshot KiriImageView::renderSnapshot() const
{
    if (m_document == nullptr) {
        return {};
    }

    return m_document->renderSnapshot(displayedPageRole());
}

QSizeF KiriImageView::viewportSize() const
{
    return m_document == nullptr ? QSizeF() : m_document->viewportSize();
}

QSizeF KiriImageView::displaySize() const
{
    return m_document == nullptr ? QSizeF() : m_document->displaySize();
}

QRectF KiriImageView::viewportImageRect() const
{
    return KiriView::imageViewportImageRect(viewportSize(), displaySize());
}

bool KiriImageView::rightToLeftReadingActive() const
{
    return m_document != nullptr && m_document->rightToLeftReadingEnabled()
        && m_document->rightToLeftReadingAvailable();
}

void KiriImageView::connectDocument()
{
    if (m_document == nullptr) {
        return;
    }

    m_repaintConnection
        = connect(m_document, &KiriImageDocument::repaintRequested, this, &KiriImageView::update);
    m_displayedUrlChangedConnection = connect(m_document, &KiriImageDocument::displayedUrlChanged,
        this, &KiriImageView::handleDisplayedUrlChanged);
    m_loadingChangedConnection = connect(
        m_document, &KiriImageDocument::loadingChanged, this, &KiriImageView::handleLoadingChanged);
    m_documentDestroyedConnection = connect(m_document, &QObject::destroyed, this, [this]() {
        m_document = nullptr;
        m_renderContextBinding.reset();
        m_repaintConnection = {};
        m_displayedUrlChangedConnection = {};
        m_loadingChangedConnection = {};
        m_documentDestroyedConnection = {};
        Q_EMIT documentChanged();
        update();
    });
    applyRenderContextBinding(m_renderContextBinding.setDocumentAttached(true), m_document);
}

void KiriImageView::disconnectDocument()
{
    KiriImageDocument *document = m_document;
    m_document = nullptr;
    QObject::disconnect(m_repaintConnection);
    QObject::disconnect(m_displayedUrlChangedConnection);
    QObject::disconnect(m_loadingChangedConnection);
    QObject::disconnect(m_documentDestroyedConnection);
    m_repaintConnection = {};
    m_displayedUrlChangedConnection = {};
    m_loadingChangedConnection = {};
    m_documentDestroyedConnection = {};
    applyRenderContextBinding(m_renderContextBinding.reset(), document);
}

void KiriImageView::applyRenderContextBinding(
    KiriView::ImageViewRenderContextBindingAction action, KiriImageDocument *document)
{
    if (document == nullptr) {
        return;
    }

    switch (action) {
    case KiriView::ImageViewRenderContextBindingAction::None:
        return;
    case KiriView::ImageViewRenderContextBindingAction::InstallProvider:
        document->setRenderContextProvider([this]() { return renderContext(); });
        return;
    case KiriView::ImageViewRenderContextBindingAction::ClearProvider:
        document->setRenderContextProvider({});
        return;
    }
}

void KiriImageView::handleDisplayedUrlChanged()
{
    m_scanState.beginDisplayedImage();
    Q_EMIT displayedImageInitialContentPositionRequested();
}

void KiriImageView::handleLoadingChanged()
{
    if (m_document != nullptr && !m_document->loading()) {
        m_scanState.cancelPendingDisplayedImageStart();
    }
}

KiriView::ImageDocumentRenderContext KiriImageView::renderContext() const
{
    return KiriView::ImageDocumentRenderContext { displayDevicePixelRatio(), maximumTextureSize() };
}

qreal KiriImageView::displayDevicePixelRatio() const
{
    const QQuickWindow *quickWindow = window();
    if (quickWindow == nullptr) {
        return 1.0;
    }

    const qreal devicePixelRatio = quickWindow->effectiveDevicePixelRatio();
    if (!std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return 1.0;
    }

    return devicePixelRatio;
}

int KiriImageView::maximumTextureSize() const
{
    const QQuickWindow *quickWindow = window();
    QRhi *rhi = quickWindow == nullptr ? nullptr : quickWindow->rhi();
    if (rhi == nullptr) {
        return KiriView::fallbackTextureSizeMax;
    }

    const int maximumTextureSize = rhi->resourceLimit(QRhi::TextureSizeMax);
    return maximumTextureSize > 0 ? maximumTextureSize : KiriView::fallbackTextureSizeMax;
}
