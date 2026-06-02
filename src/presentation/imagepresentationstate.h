// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONSTATE_H
#define KIRIVIEW_IMAGEPRESENTATIONSTATE_H

namespace KiriView {
enum class ImagePresentationTransitionState {
    PreviousActive,
    TransitioningPlaceholder,
    CommittedActive,
};
}

#endif
