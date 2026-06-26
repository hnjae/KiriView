// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEITEMS_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEITEMS_H

#include "directmedianavigationmodel.h"
#include "imagedocumentpagenavigationtypes.h"

#include <KFileItem>
#include <vector>

namespace kiriview {
std::vector<ImageDocumentPageCandidate> imageDocumentPageNavigationCandidates(
    const KFileItemList& items);
std::vector<DirectMediaNavigationCandidate> directMediaNavigationCandidates(
    const KFileItemList& items);
std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList& items);
}

#endif
