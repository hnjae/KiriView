// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEWPORTCONTEXTBRIDGE_H
#define KIRIVIEW_KIRIIMAGEVIEWPORTCONTEXTBRIDGE_H

#include "facade/kiriimagedocument.h"
#include "presentation/imageviewrendercontextbinding.h"
#include "rendering/imagerendercontext.h"

#include <QMetaObject>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

class KiriImageViewportContextBridge : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(KiriImageDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(
        bool secondaryPage READ secondaryPage WRITE setSecondaryPage NOTIFY secondaryPageChanged)
    Q_PROPERTY(bool renderContextProviderInstalled READ renderContextProviderInstalled NOTIFY
            renderContextProviderInstalledChanged)

public:
    explicit KiriImageViewportContextBridge(QQuickItem *parent = nullptr);
    ~KiriImageViewportContextBridge() override;

    KiriImageDocument *document() const;
    void setDocument(KiriImageDocument *document);
    bool secondaryPage() const;
    void setSecondaryPage(bool secondaryPage);
    bool renderContextProviderInstalled() const;

    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void releaseResources() override;
    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void documentChanged();
    void secondaryPageChanged();
    void renderContextProviderInstalledChanged();

private:
    KiriView::ImageViewRenderContextBindingInput renderContextBindingInput(
        bool documentAttached) const;
    void connectDocument();
    void disconnectDocument();
    void synchronizeRenderContextBinding(KiriImageDocument *document, bool documentAttached);
    void applyRenderContextBinding(
        KiriView::ImageViewRenderContextBindingAction action, KiriImageDocument *document);
    void emitProviderInstalledChangeIfNeeded(bool previouslyInstalled);
    void invalidateRenderContext();
    KiriView::ImageDocumentRenderContext renderContext() const;
    qreal displayDevicePixelRatio() const;
    int maximumTextureSize() const;

    KiriImageDocument *m_document = nullptr;
    bool m_secondaryPage = false;
    bool m_componentComplete = true;
    quint64 m_renderContextGeneration = 1;
    QMetaObject::Connection m_documentDestroyedConnection;
    KiriView::ImageViewRenderContextBinding m_renderContextBinding;
};

#endif
