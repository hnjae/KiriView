// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCCALLBACKS_H
#define KIRIVIEW_IMAGEASYNCCALLBACKS_H

#include <QByteArray>
#include <QString>
#include <functional>

namespace kiriview {
using ImageDataCallback = std::function<void(QByteArray)>;
using ErrorCallback = std::function<void(const QString&)>;
}

#endif
