// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIACANDIDATEPROVIDER_H
#define KIRIVIEW_MEDIACANDIDATEPROVIDER_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "medianavigationmodel.h"

#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace KiriView {
using MediaCandidatesCallback = std::function<void(std::vector<MediaNavigationCandidate>)>;

struct MediaNavigationCandidateProvider {
    using MediaCandidateLoader
        = std::function<ImageIoJob(QObject *, QUrl, MediaCandidatesCallback, ErrorCallback)>;

    MediaCandidateLoader directoryMedia;
};

MediaNavigationCandidateProvider defaultMediaNavigationCandidateProvider();
MediaNavigationCandidateProvider mediaNavigationCandidateProviderWithDefault(
    MediaNavigationCandidateProvider provider);
}

#endif
