// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationstartupsourceconversion.h"

#include "application/applicationstartupsource.h"
#include "kiriview/src/policy/applicationruntime.cxx.h"

namespace {
QString rustStringToQString(const rust::String &text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

KiriView::ApplicationInitialSourceKind applicationInitialSourceKind(
    KiriView::ApplicationStartupSourceKind kind)
{
    switch (kind) {
    case KiriView::ApplicationStartupSourceKind::None:
        return KiriView::ApplicationInitialSourceKind::None;
    case KiriView::ApplicationStartupSourceKind::LocalFilePath:
        return KiriView::ApplicationInitialSourceKind::LocalFilePath;
    case KiriView::ApplicationStartupSourceKind::UrlText:
        return KiriView::ApplicationInitialSourceKind::UrlText;
    }

    return KiriView::ApplicationInitialSourceKind::None;
}
}

namespace KiriView {
ApplicationInitialSource applicationInitialSourceFromBridge(const ApplicationStartupSource &source)
{
    return ApplicationInitialSource {
        applicationInitialSourceKind(source.kind),
        rustStringToQString(source.text),
    };
}
}
