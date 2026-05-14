// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDELETIONCONTROLLER_H
#define KIRIVIEW_IMAGEDELETIONCONTROLLER_H

#include "filedeletion.h"
#include "filedeletionfallback.h"
#include "imagecandidaterepository.h"
#include "imageiojob.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>
#include <utility>
#include <variant>

namespace KiriView {
struct ClearDeletedImageAfterDeletionEffect {
};

struct OpenImageDeletionFallbackEffect {
    QUrl url;
};

struct OpenContainerImageDeletionFallbackEffect {
    QUrl imageUrl;
    QUrl containerUrl;
};

struct ReportImageDeletionFailureEffect {
    QString errorString;
};

struct ImageDeletionEffect {
    using Payload
        = std::variant<ClearDeletedImageAfterDeletionEffect, OpenImageDeletionFallbackEffect,
            OpenContainerImageDeletionFallbackEffect, ReportImageDeletionFailureEffect>;

    static ImageDeletionEffect clearDeletedImage()
    {
        return ImageDeletionEffect(ClearDeletedImageAfterDeletionEffect {});
    }

    static ImageDeletionEffect openImageFallback(const QUrl &url)
    {
        return ImageDeletionEffect(OpenImageDeletionFallbackEffect { url });
    }

    static ImageDeletionEffect openContainerImageFallback(
        const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return ImageDeletionEffect(
            OpenContainerImageDeletionFallbackEffect { imageUrl, containerUrl });
    }

    static ImageDeletionEffect reportFailure(const QString &errorString)
    {
        return ImageDeletionEffect(ReportImageDeletionFailureEffect { errorString });
    }

    explicit ImageDeletionEffect(Payload effectPayload)
        : payload(std::move(effectPayload))
    {
    }

    Payload payload;
};

class ImageDeletionController final : public QObject
{
public:
    using InProgressChangedCallback = std::function<void()>;
    using EffectCallback = std::function<void(ImageDeletionEffect)>;

    struct Callbacks {
        InProgressChangedCallback inProgressChanged;
        EffectCallback effect;
    };

    ImageDeletionController(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        FileOperationProvider fileOperationProvider, Callbacks callbacks);
    ~ImageDeletionController() override;

    bool inProgress() const;
    void deleteDisplayedFile(const DisplayedImageLocation &location, FileDeletionMode mode);
    void cancel();

private:
    void finishFileDeletion(const DeletionFallbackPlan &fallbackPlan, FileDeletionResult result,
        const QString &errorString);
    void openDeletionFallback(const DeletionFallbackPlan &fallbackPlan);
    void openDeletionFallbackPlan(const NoDeletionFallbackPlan &fallbackPlan);
    void openDeletionFallbackPlan(const ImageDeletionFallbackPlan &fallbackPlan);
    void openDeletionFallbackPlan(const ComicBookDeletionFallbackPlan &fallbackPlan);
    void openComicBookDeletionFallbackCandidate(
        const std::optional<ContainerNavigationCandidate> &candidate,
        const std::optional<ContainerNavigationCandidate> &fallbackCandidate);
    void setInProgress(bool inProgress);
    void cancelFileDeletion();
    void cancelFallback();
    void report(ImageDeletionEffect effect);
    void reportFailure(const QString &errorString);

    Callbacks m_callbacks;
    ImageCandidateRepository m_candidateRepository;
    FileOperationProvider m_fileOperationProvider;
    ImageIoJob m_fileDeletionJob;
    ImageIoJob m_fallbackJob;
    bool m_inProgress = false;
};
}

#endif
