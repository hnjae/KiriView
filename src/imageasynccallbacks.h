// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCCALLBACKS_H
#define KIRIVIEW_IMAGEASYNCCALLBACKS_H

#include "imagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <functional>
#include <vector>

namespace KiriView {
using ImageCandidatesCallback = std::function<void(std::vector<ImageNavigationCandidate>)>;
using ContainerCandidatesCallback = std::function<void(std::vector<ContainerNavigationCandidate>)>;
using ImageDataCallback = std::function<void(QByteArray)>;
using ErrorCallback = std::function<void(const QString &)>;
}

#endif
