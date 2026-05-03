// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "imageasyncdependencies.h"
#include "imageasyncticket.h"
#include "imagecandidaterepository.h"
#include "imagedecodejob.h"
#include "imageiojob.h"
#include "imagelocation.h"
#include "predecodecache.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
class ImagePredecodeCoordinator final : public QObject
{
public:
    struct Context {
        DisplayedImageLocation displayedImageLocation;
        bool displayedImageIsCacheable = false;
        QImage displayedImage;
    };

    explicit ImagePredecodeCoordinator(QObject *parent = nullptr);
    ImagePredecodeCoordinator(QObject *parent, const ImageAsyncDependencies &dependencies);

    void schedule(Context context);
    void cancel();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    void scheduleAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls,
        const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation);
    void startNextPredecodeImageLoad(quint64 generation);
    void startPredecodeImageLoad(
        const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation);
    void finishPredecodeImageLoadError(const ImageDecodeRequest &request);
    void finishPredecodeImageDecode(ImageDecodeRequest request, const DecodedImageResult &result);

    ImageIoJob m_listerJob;
    ImageDecodeJob m_decodeJob;
    ImageCandidateRepository m_candidateRepository;
    PredecodeCache m_cache;
    QUrl m_activePredecodeUrl;
    ArchiveDocumentLocation m_activePredecodeArchiveDocument;
    ImageAsyncTicket m_generation;
};
}

#endif
