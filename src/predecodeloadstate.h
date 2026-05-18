// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODELOADSTATE_H
#define KIRIVIEW_PREDECODELOADSTATE_H

#include "imagedecoderequest.h"
#include "predecodeactivedecodestore.h"
#include "predecodecache.h"
#include "predecodedimage.h"
#include "staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
class ImageDecodeJob;

struct PredecodeLoadWindow {
    QUrl primaryDisplayedUrl;
    ArchiveDocumentLocation archiveDocument;
    std::vector<QUrl> urls;
    std::vector<DisplayedPredecodeImage> displayedImages;
    ImageFirstDisplayDecodeContext firstDisplayContext;
    quint64 generation = 0;
    std::size_t parallelLimit = 0;
};

struct PredecodeLoadStart {
    ImageDecodeRequest request;
};

class PredecodeLoadState final
{
public:
    void cacheDisplayedImages(const std::vector<DisplayedPredecodeImage> &images);
    void clearWindowUrls();
    void startWindow(PredecodeLoadWindow window);
    bool canStartMoreLoads() const;
    std::optional<PredecodeLoadStart> takeNextLoad();
    void addActiveLoad(ImageDecodeRequest request, ImageDecodeJob *decodeJob);
    std::optional<ImageDecodeRequest> finishActiveLoad(const ImageDecodeRequest &request);
    void cacheDecodedImage(const ImageDecodeRequest &request, StaticImagePayload staticImage);
    void cancelBackgroundWork();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    struct ActiveWindow {
        ImageFirstDisplayDecodeContext firstDisplayContext;
        quint64 generation = 0;
        std::size_t parallelLimit = 0;
    };

    std::optional<ActiveWindow> m_activeWindow;
    PredecodeCache m_cache;
    PredecodeActiveDecodeStore m_activeRequests;
};
}

#endif
