// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEDOCUMENTSESSIONSTORE_H
#define KIRIVIEW_ARCHIVEDOCUMENTSESSIONSTORE_H

#include "archivebackend.h"
#include "archivedocumentsessionruntime.h"
#include "async/imageiojob.h"
#include "decoding/imagedecodedependencies.h"
#include "navigation/imagecandidateprovider.h"

#include <QObject>

namespace KiriView {
struct ImageDocumentSourceLoadRequest;

class ArchiveDocumentSessionStore final : public QObject
{
public:
    explicit ArchiveDocumentSessionStore(
        ArchiveDocumentSessionFactory sessionFactory = {}, QObject *parent = nullptr);
    ~ArchiveDocumentSessionStore() override;

    ImageNavigationCandidateProvider wrapCandidateProvider(
        ImageNavigationCandidateProvider provider);
    ImageDecodeDependencies wrapDecodeDependencies(ImageDecodeDependencies dependencies);

    void prepareForSourceLoad(const ImageDocumentSourceLoadRequest &request,
        const ImagePageScopeLocation &displayedImagePageScope);
    void clear();

    bool hasCurrentArchiveDocument() const;
    bool hasCurrentArchiveDocument(const ImagePageScopeLocation &archiveDocument) const;

    ImageIoJob loadArchiveImages(QObject *receiver, ImagePageScopeLocation archiveDocument,
        ImageCandidatesCallback callback, ErrorCallback errorCallback);
    ImageIoJob loadArchiveImageData(QObject *receiver, ImageDecodeRequest request,
        ImageDataCallback callback, ErrorCallback errorCallback);

private:
    ArchiveDocumentSessionRuntime m_runtime;
};
}

#endif
