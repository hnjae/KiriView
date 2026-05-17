// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCHANGEBATCHER_H
#define KIRIVIEW_IMAGEDOCUMENTCHANGEBATCHER_H

#include "imagedocumenttypes.h"

#include <functional>
#include <vector>

namespace KiriView {
class ImageDocumentChangeBatcher final
{
public:
    using ChangeCallback = std::function<void(ImageDocumentChange)>;

    class Batch final
    {
    public:
        explicit Batch(ImageDocumentChangeBatcher &batcher);
        ~Batch();

        Batch(const Batch &) = delete;
        Batch &operator=(const Batch &) = delete;
        Batch(Batch &&other) noexcept;
        Batch &operator=(Batch &&) = delete;

    private:
        ImageDocumentChangeBatcher *m_batcher = nullptr;
    };

    explicit ImageDocumentChangeBatcher(ChangeCallback changeCallback = {});

    Batch beginBatch();
    void notify(ImageDocumentChange change);
    void notifyAll(const std::vector<ImageDocumentChange> &changes);

private:
    void begin();
    void end();
    void emitChange(ImageDocumentChange change);

    ChangeCallback m_changeCallback;
    int m_batchDepth = 0;
    std::vector<ImageDocumentChange> m_pendingChanges;
};
}

#endif
