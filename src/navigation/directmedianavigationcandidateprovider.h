// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DIRECTMEDIANAVIGATIONCANDIDATEPROVIDER_H
#define KIRIVIEW_DIRECTMEDIANAVIGATIONCANDIDATEPROVIDER_H

#include "async/directorylistingjob.h"
#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "directmedianavigationmodel.h"

#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace KiriView {
using DirectMediaNavigationCandidatesCallback
    = std::function<void(std::vector<DirectMediaNavigationCandidate>)>;

struct DirectMediaNavigationCandidateProvider {
    using DirectMediaNavigationCandidateLoader = std::function<ImageIoJob(
        QObject *, QUrl, DirectMediaNavigationCandidatesCallback, ErrorCallback)>;

    DirectMediaNavigationCandidateLoader directoryCandidateLoader;
};

DirectMediaNavigationCandidateProvider defaultDirectMediaNavigationCandidateProvider(
    DirectoryItemListProvider directoryItemListProvider = {});
DirectMediaNavigationCandidateProvider directMediaNavigationCandidateProviderWithDefault(
    DirectMediaNavigationCandidateProvider provider,
    DirectoryItemListProvider directoryItemListProvider = {});
}

#endif
