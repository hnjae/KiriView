// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODELOADCONTROLLER_H
#define KIRIVIEW_PREDECODELOADCONTROLLER_H

#include "decoding/imagedecodedependencies.h"
#include "predecodeactivedecodestore.h"
#include "predecodedimage.h"
#include "predecodeloadstate.h"

#include <QUrl>
#include <optional>

class QObject;

namespace KiriView {
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
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

private:
    void startNextLoads();
    bool startLoad(PredecodeLoadStart load);
    void finishLoadError(const ImageDecodeRequest &request);
    void finishDecode(ImageDecodeRequest request, const DecodedImageResult &result);

    QObject *m_parent = nullptr;
    ImageDecodeDependencies m_decodeDependencies;
    PredecodeLoadState m_loadState;
    PredecodeActiveDecodeStore m_activeDecodes;
};
}

#endif
