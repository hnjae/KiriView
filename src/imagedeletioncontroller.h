// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDELETIONCONTROLLER_H
#define KIRIVIEW_IMAGEDELETIONCONTROLLER_H

#include "filedeletion.h"
#include "imagecandidaterepository.h"
#include "imagedeletioneffects.h"
#include "imageiojob.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"
#include "imageremovalfallback.h"

#include <QObject>
#include <functional>
#include <optional>

namespace KiriView {
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
    void finishFileDeletion(const ImageRemovalFallbackPlan &fallbackPlan, FileDeletionResult result,
        const QString &errorString);
    void openRemovalFallback(const ImageRemovalFallbackPlan &fallbackPlan);
    void openFallbackPlan(const NoImageRemovalFallback &);
    void openFallbackPlan(const ImageRemovalFallback &fallback);
    void openFallbackPlan(const ComicBookRemovalFallback &fallback);
    void openComicBookFallbackCandidate(
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
