// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEACTIVEDECODESTORE_H
#define KIRIVIEW_PREDECODEACTIVEDECODESTORE_H

#include "imagedecoderequest.h"

#include <QPointer>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
class ImageDecodeJob;

class PredecodeActiveDecodeStore final
{
public:
    ~PredecodeActiveDecodeStore();

    void add(ImageDecodeRequest request, ImageDecodeJob *decodeJob);
    std::size_t size() const;
    bool containsUrl(const QUrl &url) const;
    std::vector<QUrl> urls() const;
    std::optional<ImageDecodeRequest> finish(const ImageDecodeRequest &request);
    void cancel();

private:
    struct Entry {
        ImageDecodeRequest request;
        QUrl normalizedUrl;
        QPointer<ImageDecodeJob> decodeJob;
    };

    std::vector<Entry> m_entries;
};
}

#endif
