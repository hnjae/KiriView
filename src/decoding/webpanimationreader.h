// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_WEBPANIMATIONREADER_H
#define KIRIVIEW_WEBPANIMATIONREADER_H

#include "animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <memory>
#include <optional>

namespace kiriview {
enum class WebPAnimationOpenStatus {
    NotWebP,
    NotAnimation,
    Success,
    Error,
};

struct WebPAnimationOpenResult
{
    WebPAnimationOpenStatus status = WebPAnimationOpenStatus::NotWebP;
    QImage firstFrame;
    int firstFrameDelay = 0;
    int loopCount = 0;
    bool sourceHasMoreFrames = false;
    QString errorString;
};

class WebPAnimationReader final
{
public:
    WebPAnimationReader();
    ~WebPAnimationReader();

    WebPAnimationReader(const WebPAnimationReader&) = delete;
    WebPAnimationReader& operator=(const WebPAnimationReader&) = delete;
    WebPAnimationReader(WebPAnimationReader&&) noexcept;
    WebPAnimationReader& operator=(WebPAnimationReader&&) noexcept;

    WebPAnimationOpenResult open(QByteArray data);
    std::optional<AnimationFrame> readNextFrame(QString* errorString);
    bool hasMoreFrames() const;
    void close();

private:
    class Private;
    std::unique_ptr<Private> d;
};
}

#endif
