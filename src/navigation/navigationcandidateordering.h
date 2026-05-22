// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_NAVIGATIONCANDIDATEORDERING_H
#define KIRIVIEW_NAVIGATIONCANDIDATEORDERING_H

#include "location/imageurl.h"

#include <QCollator>
#include <QLocale>
#include <Qt>
#include <algorithm>
#include <vector>

namespace KiriView {
template <typename Candidate>
void sortNavigationCandidatesByNameAndUrl(std::vector<Candidate> *candidates)
{
    QCollator collator(QLocale::system());
    collator.setCaseSensitivity(Qt::CaseSensitive);
    collator.setNumericMode(false);
    collator.setIgnorePunctuation(false);

    std::stable_sort(candidates->begin(), candidates->end(),
        [&collator](const Candidate &left, const Candidate &right) {
            const int nameComparison = collator.compare(left.name, right.name);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }

            return left.url.toEncoded() < right.url.toEncoded();
        });

    const auto duplicateStart = std::unique(
        candidates->begin(), candidates->end(), [](const Candidate &left, const Candidate &right) {
            return sameNormalizedUrl(left.url, right.url);
        });
    candidates->erase(duplicateStart, candidates->end());
}
}

#endif
