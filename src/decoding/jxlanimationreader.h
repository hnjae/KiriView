// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_JXLANIMATIONREADER_H
#define KIRIVIEW_JXLANIMATIONREADER_H

#include "animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <memory>
#include <optional>

namespace KiriView {
enum class JxlAnimationOpenStatus {
    NotJxl,
    NotAnimation,
    Success,
    Error,
};

struct JxlAnimationOpenResult {
    JxlAnimationOpenStatus status = JxlAnimationOpenStatus::NotJxl;
    QImage firstFrame;
    int firstFrameDelay = 0;
    int loopCount = 0;
    bool sourceHasMoreFrames = false;
    QString errorString;
};

class JxlAnimationReader final
{
public:
    JxlAnimationReader();
    ~JxlAnimationReader();

    JxlAnimationReader(const JxlAnimationReader &) = delete;
    JxlAnimationReader &operator=(const JxlAnimationReader &) = delete;
    JxlAnimationReader(JxlAnimationReader &&) noexcept;
    JxlAnimationReader &operator=(JxlAnimationReader &&) noexcept;

    JxlAnimationOpenResult open(QByteArray data);
    std::optional<AnimationFrame> readNextFrame(QString *errorString);
    void close();

private:
    class Private;
    std::unique_ptr<Private> d;
};
}

#endif
