// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_AVIFCOMPAT_H
#define KIRIVIEW_AVIFCOMPAT_H

#include <QByteArray>

namespace KiriView {
QByteArray avifDataWithCompatibilityFixes(const QByteArray &data);
}

#endif
