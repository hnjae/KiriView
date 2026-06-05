// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimageviewportcontextbridge.h"

#include "facade/kiriimagedocument.h"
#include "rendering/imagerendering.h"

#include <QQuickWindow>
#include <cmath>

KiriImageViewportContextBridge::KiriImageViewportContextBridge(QQuickItem *parent)
    : QQuickItem(parent)
{
}

KiriImageViewportContextBridge::~KiriImageViewportContextBridge() { disconnectDocument(); }

KiriImageDocument *KiriImageViewportContextBridge::document() const { return m_document; }

void KiriImageViewportContextBridge::setDocument(KiriImageDocument *document)
{
    if (m_document == document) {
        return;
    }

    disconnectDocument();
    m_document = document;
    connectDocument();
    Q_EMIT documentChanged();
}

bool KiriImageViewportContextBridge::secondaryPage() const { return m_secondaryPage; }

void KiriImageViewportContextBridge::setSecondaryPage(bool secondaryPage)
{
    if (m_secondaryPage == secondaryPage) {
        return;
    }

    m_secondaryPage = secondaryPage;
    synchronizeRenderContextBinding(m_document, m_document != nullptr);
    Q_EMIT secondaryPageChanged();
}

bool KiriImageViewportContextBridge::renderContextProviderInstalled() const
{
    return m_renderContextBinding.providerInstalled();
}

void KiriImageViewportContextBridge::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);

    if (change == ItemSceneChange || change == ItemDevicePixelRatioHasChanged) {
        invalidateRenderContext();
    }
}

void KiriImageViewportContextBridge::releaseResources()
{
    QQuickItem::releaseResources();
    invalidateRenderContext();
}

void KiriImageViewportContextBridge::classBegin()
{
    QQuickItem::classBegin();
    m_componentComplete = false;
    synchronizeRenderContextBinding(m_document, m_document != nullptr);
}

void KiriImageViewportContextBridge::componentComplete()
{
    QQuickItem::componentComplete();
    m_componentComplete = true;
    synchronizeRenderContextBinding(m_document, m_document != nullptr);
}

KiriView::ImageViewRenderContextBindingInput
KiriImageViewportContextBridge::renderContextBindingInput(bool documentAttached) const
{
    return KiriView::ImageViewRenderContextBindingInput {
        documentAttached,
        m_secondaryPage,
        m_componentComplete,
    };
}

void KiriImageViewportContextBridge::connectDocument()
{
    if (m_document == nullptr) {
        return;
    }

    m_documentDestroyedConnection = connect(m_document, &QObject::destroyed, this, [this]() {
        m_document = nullptr;
        m_documentDestroyedConnection = {};
        const bool previouslyInstalled = m_renderContextBinding.providerInstalled();
        m_renderContextBinding.synchronize(renderContextBindingInput(false));
        emitProviderInstalledChangeIfNeeded(previouslyInstalled);
        Q_EMIT documentChanged();
    });
    synchronizeRenderContextBinding(m_document, true);
}

void KiriImageViewportContextBridge::disconnectDocument()
{
    KiriImageDocument *document = m_document;
    m_document = nullptr;
    QObject::disconnect(m_documentDestroyedConnection);
    m_documentDestroyedConnection = {};
    synchronizeRenderContextBinding(document, false);
}

void KiriImageViewportContextBridge::synchronizeRenderContextBinding(
    KiriImageDocument *document, bool documentAttached)
{
    const bool previouslyInstalled = m_renderContextBinding.providerInstalled();
    applyRenderContextBinding(
        m_renderContextBinding.synchronize(renderContextBindingInput(documentAttached)), document);
    emitProviderInstalledChangeIfNeeded(previouslyInstalled);
}

void KiriImageViewportContextBridge::applyRenderContextBinding(
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

void KiriImageViewportContextBridge::emitProviderInstalledChangeIfNeeded(bool previouslyInstalled)
{
    if (previouslyInstalled != m_renderContextBinding.providerInstalled()) {
        Q_EMIT renderContextProviderInstalledChanged();
    }
}

void KiriImageViewportContextBridge::invalidateRenderContext()
{
    ++m_renderContextGeneration;
    if (m_document != nullptr) {
        m_document->updateRenderContext();
    }
}

KiriView::ImageDocumentRenderContext KiriImageViewportContextBridge::renderContext() const
{
    return KiriView::ImageDocumentRenderContext {
        displayDevicePixelRatio(),
        maximumTextureSize(),
        m_renderContextGeneration,
    };
}

qreal KiriImageViewportContextBridge::displayDevicePixelRatio() const
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

int KiriImageViewportContextBridge::maximumTextureSize() const
{
    return KiriView::fallbackTextureSizeMax;
}
