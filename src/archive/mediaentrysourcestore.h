// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCESTORE_H
#define KIRIVIEW_MEDIAENTRYSOURCESTORE_H

#include "archivebackend.h"
#include "async/imageiojob.h"
#include "decoding/imagedecodedependencies.h"
#include "mediaentrysourceruntime.h"
#include "navigation/imagecandidateprovider.h"

#include <QObject>

namespace KiriView {
struct ImageDocumentSourceLoadRequest;

class MediaEntrySourceStore final : public QObject
{
public:
    explicit MediaEntrySourceStore(
        MediaEntrySourceFactory sourceFactory = {}, QObject *parent = nullptr);
    ~MediaEntrySourceStore() override;

    ImageNavigationCandidateProvider wrapCandidateProvider(
        ImageNavigationCandidateProvider provider);
    ImageDecodeDependencies wrapDecodeDependencies(ImageDecodeDependencies dependencies);

    void prepareForSourceLoad(const ImageDocumentSourceLoadRequest &request,
        const OpenedCollectionScopeLocation &displayedOpenedCollectionScope);
    void clear();

    bool hasCurrentOpenedCollectionScope() const;
    bool hasCurrentOpenedCollectionScope(
        const OpenedCollectionScopeLocation &archiveCollection) const;

    ImageIoJob loadOpenedCollectionCandidates(QObject *receiver,
        OpenedCollectionScopeLocation archiveCollection, ImageCandidatesCallback callback,
        ErrorCallback errorCallback);
    ImageIoJob loadOpenedCollectionImageData(QObject *receiver, ImageDecodeRequest request,
        ImageDataCallback callback, ErrorCallback errorCallback);

private:
    MediaEntrySourceRuntime m_runtime;
};
}

#endif
