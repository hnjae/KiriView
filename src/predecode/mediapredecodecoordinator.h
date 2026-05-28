// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODECOORDINATOR_H
#define KIRIVIEW_MEDIAPREDECODECOORDINATOR_H

#include "decoding/imagedecodedependencies.h"
#include "navigation/directmedianavigationmodel.h"
#include "predecodedimage.h"
#include "predecodeloadcontroller.h"
#include "predecodescheduleruntime.h"
#include "system/powersaverprovider.h"

#include <QObject>
#include <QUrl>
#include <QtGlobal>
#include <memory>
#include <optional>
#include <vector>

namespace KiriView {
class MediaPredecodeCoordinator final : public QObject
{
public:
    struct Context {
        QUrl currentUrl;
        std::vector<DirectMediaNavigationCandidate> candidates;
        std::vector<DisplayedPredecodeImage> displayedImages;
        ImageFirstDisplayDecodeContext firstDisplayContext;
    };

    explicit MediaPredecodeCoordinator(QObject *parent = nullptr);
    MediaPredecodeCoordinator(QObject *parent, ImageDecodeDependencies decodeDependencies,
        PowerSaverProvider powerSaverProvider, qsizetype cacheByteBudget);

    void schedule(Context context);
    void setPowerSaverEnabled(bool enabled);
    bool powerSaverEnabled() const;
    void cancel();
    void clear();
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

private:
    void startPredecodeWindow(const PredecodePendingSchedule &schedule);
    PredecodePolicyInput policyInput() const;

    PredecodeLoadController m_loadController;
    PredecodeScheduleRuntime m_scheduleRuntime;
};
}

#endif
