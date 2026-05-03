// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDECODER_H
#define KIRIVIEW_KIRIIMAGEDECODER_H

#include "animationframe.h"
#include "decodedimageresult.h"

#include <QByteArray>
#include <QString>
#include <memory>
#include <optional>

namespace KiriView {
enum class HeifSequenceOpenStatus {
    NotHeif,
    NotSequence,
    Success,
    Error,
};

struct HeifSequenceOpenResult {
    HeifSequenceOpenStatus status = HeifSequenceOpenStatus::NotHeif;
    QString errorString;
};

class HeifSequenceReader final
{
public:
    HeifSequenceReader();
    ~HeifSequenceReader();

    HeifSequenceReader(const HeifSequenceReader &) = delete;
    HeifSequenceReader &operator=(const HeifSequenceReader &) = delete;
    HeifSequenceReader(HeifSequenceReader &&) noexcept;
    HeifSequenceReader &operator=(HeifSequenceReader &&) noexcept;

    HeifSequenceOpenResult open(QByteArray data);
    std::optional<AnimationFrame> readNextFrame(QString *errorString);
    void close();

private:
    class Private;
    std::unique_ptr<Private> d;
};

DecodedImageResult decodeImageData(const QByteArray &data);
}

#endif
