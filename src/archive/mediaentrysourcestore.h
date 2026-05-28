// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCESTORE_H
#define KIRIVIEW_MEDIAENTRYSOURCESTORE_H

#include "async/imageiojob.h"
#include "decoding/imagedecodedependencies.h"
#include "mediaentrysourcebackend.h"
#include "mediaentrysourceruntime.h"
#include "navigation/imagedocumentpagecandidateprovider.h"

#include <QObject>

namespace KiriView {
struct ImageDocumentSourceLoadRequest;

class MediaEntrySourceStore final : public QObject
{
public:
    explicit MediaEntrySourceStore(
        MediaEntrySourceFactory sourceFactory = {}, QObject *parent = nullptr);
    ~MediaEntrySourceStore() override;

    ImageDocumentPageCandidateProvider wrapCandidateProvider(
        ImageDocumentPageCandidateProvider provider);
    ImageDecodeDependencies wrapDecodeDependencies(ImageDecodeDependencies dependencies);

    void prepareForSourceLoad(const ImageDocumentSourceLoadRequest &request,
        const OpenedCollectionScopeLocation &displayedOpenedCollectionScope);
    void clear();

    bool hasCurrentOpenedCollectionScope() const;
    bool hasCurrentOpenedCollectionScope(
        const OpenedCollectionScopeLocation &openedCollectionScope) const;

    ImageIoJob loadOpenedCollectionCandidates(QObject *receiver,
        OpenedCollectionScopeLocation openedCollectionScope,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
    ImageIoJob loadOpenedCollectionImageData(QObject *receiver, ImageDecodeRequest request,
        ImageDataCallback callback, ErrorCallback errorCallback);

private:
    MediaEntrySourceRuntime m_runtime;
};
}

#endif
