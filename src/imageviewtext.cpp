// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewtext.h"

#include <QCoreApplication>

namespace KiriView {
QString imageViewText(const char *sourceText)
{
    return QCoreApplication::translate("KiriImageView", sourceText);
}
}
