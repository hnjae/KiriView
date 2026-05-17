// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODELOADCONTROLLER_H
#define KIRIVIEW_PREDECODELOADCONTROLLER_H

#include "imageasyncdependencies.h"
#include "predecodeactivedecodestore.h"
#include "predecodecache.h"
#include "predecodedimage.h"
#include "staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <cstddef>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
struct PredecodeLoadWindow {
    QUrl primaryDisplayedUrl;
    ArchiveDocumentLocation archiveDocument;
    std::vector<QUrl> urls;
    std::vector<DisplayedPredecodeImage> displayedImages;
    ImageFirstDisplayDecodeContext firstDisplayContext;
    quint64 generation = 0;
    std::size_t parallelLimit = 0;
};

class PredecodeLoadController final
{
public:
    explicit PredecodeLoadController(QObject *parent = nullptr);
    PredecodeLoadController(QObject *parent, ImageDecodeDependencies decodeDependencies);
    ~PredecodeLoadController();

    void cacheDisplayedImages(const std::vector<DisplayedPredecodeImage> &images);
    void clearWindowUrls();
    void startWindowLoads(PredecodeLoadWindow window);
    void cancelBackgroundWork();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    void startNextLoads(quint64 generation);
    bool startLoad(
        const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation);
    void finishLoadError(const ImageDecodeRequest &request);
    void finishDecode(ImageDecodeRequest request, const DecodedImageResult &result);

    QObject *m_parent = nullptr;
    ImageDecodeDependencies m_decodeDependencies;
    PredecodeCache m_cache;
    PredecodeActiveDecodeStore m_activeRequests;
    ImageFirstDisplayDecodeContext m_firstDisplayContext;
    std::size_t m_parallelLimit = 1;
};
}

#endif
