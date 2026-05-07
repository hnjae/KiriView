// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewtext.h"

#include <KLocalizedString>

namespace KiriView {
QString imageViewText(const char *sourceText) { return i18n(sourceText); }
}
