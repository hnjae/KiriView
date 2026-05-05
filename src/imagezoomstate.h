// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEZOOMSTATE_H
#define KIRIVIEW_IMAGEZOOMSTATE_H

#include <QSize>
#include <QSizeF>
#include <QUrl>

namespace KiriView {
struct RustImageZoomState;

enum class ImageZoomMode {
    Fit,
    FitHeight,
    FitWidth,
    Manual,
};

struct ImageZoomSnapshot {
    QSize imageSize;
    QSizeF viewportSize;
    QSizeF displaySize;
    qreal zoomPercent = 100.0;
    ImageZoomMode zoomMode = ImageZoomMode::Fit;
    QUrl containerUrl;
};

struct LoadedImageZoom {
    ImageZoomMode zoomMode = ImageZoomMode::Fit;
    qreal zoomPercent = 100.0;
    QSizeF displaySize;
};

class ImageZoomState
{
public:
    static qreal minimumManualZoomPercent();
    static qreal maximumManualZoomPercent();
    static int manualZoomStepPercent();

    const QSize &imageSize() const;
    const QSizeF &viewportSize() const;
    const QSizeF &displaySize() const;
    qreal zoomPercent() const;
    ImageZoomMode zoomMode() const;
    const QUrl &containerUrl() const;
    ImageZoomSnapshot snapshot() const;

    bool setViewportSize(const QSizeF &viewportSize, qreal devicePixelRatio);
    bool setImageSize(const QSize &imageSize, qreal devicePixelRatio);
    bool setManualZoomPercent(qreal zoomPercent, qreal devicePixelRatio);
    bool setFitMode(ImageZoomMode zoomMode, qreal devicePixelRatio);
    void resetZoom(qreal devicePixelRatio);
    void prepareImageContainer(const QUrl &containerUrl);
    LoadedImageZoom loadedImageZoom(
        const QUrl &containerUrl, const QSize &imageSize, qreal devicePixelRatio) const;
    void setLoadedSvgZoom(const QUrl &containerUrl, const LoadedImageZoom &zoom);
    void clearContainer();
    void update(qreal devicePixelRatio);

    qreal fitZoomPercent(ImageZoomMode zoomMode, qreal devicePixelRatio) const;
    qreal fitZoomPercentForImageSize(
        ImageZoomMode zoomMode, const QSize &imageSize, qreal devicePixelRatio) const;
    QSizeF displaySizeForZoomPercent(qreal zoomPercent, qreal devicePixelRatio) const;
    QSizeF displaySizeForZoomPercent(
        qreal zoomPercent, const QSize &imageSize, qreal devicePixelRatio) const;

private:
    RustImageZoomState rustState() const;
    void applyRustState(const RustImageZoomState &state);

    QSize m_imageSize;
    QSizeF m_viewportSize;
    QSizeF m_displaySize;
    qreal m_zoomPercent = 100.0;
    ImageZoomMode m_zoomMode = ImageZoomMode::Fit;
    QUrl m_containerUrl;
};

bool imageZoomApproximatelyEqual(qreal left, qreal right);
bool imageZoomApproximatelyEqual(const QSizeF &left, const QSizeF &right);
}

#endif
