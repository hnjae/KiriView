// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_FACADE_IMAGEACTIONAVAILABILITY_H
#define KIRIVIEW_FACADE_IMAGEACTIONAVAILABILITY_H

#include "application/imageactionavailabilityruntime.h"

#include <QObject>
#include <QtQml/qqmlregistration.h>

class ImageActionAvailability : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool imageReady READ imageReady WRITE setImageReady NOTIFY availabilityChanged)
    Q_PROPERTY(bool fileDeletionInProgress READ fileDeletionInProgress WRITE
            setFileDeletionInProgress NOTIFY availabilityChanged)
    Q_PROPERTY(
        bool helpDialogOpen READ helpDialogOpen WRITE setHelpDialogOpen NOTIFY availabilityChanged)
    Q_PROPERTY(bool textInputFocused READ textInputFocused WRITE setTextInputFocused NOTIFY
            availabilityChanged)
    Q_PROPERTY(
        bool imagePannable READ imagePannable WRITE setImagePannable NOTIFY availabilityChanged)
    Q_PROPERTY(bool imageHorizontallyPannable READ imageHorizontallyPannable WRITE
            setImageHorizontallyPannable NOTIFY availabilityChanged)
    Q_PROPERTY(bool containerNavigationAvailable READ containerNavigationAvailable WRITE
            setContainerNavigationAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(bool twoPageModeEnabled READ twoPageModeEnabled WRITE setTwoPageModeEnabled NOTIFY
            availabilityChanged)
    Q_PROPERTY(bool twoPageModeAvailable READ twoPageModeAvailable WRITE setTwoPageModeAvailable
            NOTIFY availabilityChanged)
    Q_PROPERTY(bool rightToLeftReadingEnabled READ rightToLeftReadingEnabled WRITE
            setRightToLeftReadingEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool rightToLeftReadingAvailable READ rightToLeftReadingAvailable WRITE
            setRightToLeftReadingAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(bool scanBackwardAtFirstImageBoundary READ scanBackwardAtFirstImageBoundary WRITE
            setScanBackwardAtFirstImageBoundary NOTIFY availabilityChanged)

    Q_PROPERTY(bool canUseReadyActions READ canUseReadyActions NOTIFY availabilityChanged)
    Q_PROPERTY(bool canUseRotateActions READ canUseRotateActions NOTIFY availabilityChanged)
    Q_PROPERTY(
        bool canUseTwoPageModeActions READ canUseTwoPageModeActions NOTIFY availabilityChanged)
    Q_PROPERTY(bool canUseRightToLeftReadingActions READ canUseRightToLeftReadingActions NOTIFY
            availabilityChanged)
    Q_PROPERTY(
        bool rightToLeftReadingActive READ rightToLeftReadingActive NOTIFY availabilityChanged)
    Q_PROPERTY(bool twoPageModeActive READ twoPageModeActive NOTIFY availabilityChanged)
    Q_PROPERTY(bool helpShortcutsEnabled READ helpShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool viewerShortcutsEnabled READ viewerShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool readyShortcutsEnabled READ readyShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool readyViewerShortcutsEnabled READ readyViewerShortcutsEnabled NOTIFY
            availabilityChanged)
    Q_PROPERTY(bool twoPageViewerShortcutsEnabled READ twoPageViewerShortcutsEnabled NOTIFY
            availabilityChanged)
    Q_PROPERTY(bool rightToLeftReadingShortcutsEnabled READ rightToLeftReadingShortcutsEnabled
            NOTIFY availabilityChanged)
    Q_PROPERTY(bool rightToLeftReadingViewerShortcutsEnabled READ
            rightToLeftReadingViewerShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool rotateShortcutsEnabled READ rotateShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool rotateViewerShortcutsEnabled READ rotateViewerShortcutsEnabled NOTIFY
            availabilityChanged)
    Q_PROPERTY(
        bool pannableShortcutsEnabled READ pannableShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool pannableViewerShortcutsEnabled READ pannableViewerShortcutsEnabled NOTIFY
            availabilityChanged)
    Q_PROPERTY(
        bool containerShortcutsEnabled READ containerShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool containerViewerShortcutsEnabled READ containerViewerShortcutsEnabled NOTIFY
            availabilityChanged)
    Q_PROPERTY(int availabilityRevision READ availabilityRevision NOTIFY availabilityChanged)

public:
    enum ShortcutScope {
        HelpShortcutScope = 0,
        ViewerShortcutScope,
        ReadyShortcutScope,
        ReadyViewerShortcutScope,
        ImageSelectionShortcutScope,
        ImageSelectionViewerShortcutScope,
        PageShortcutScope,
        PageViewerShortcutScope,
        RightToLeftReadingShortcutScope,
        RightToLeftReadingViewerShortcutScope,
        RotateShortcutScope,
        RotateViewerShortcutScope,
        PannableShortcutScope,
        PannableViewerShortcutScope,
        ContainerShortcutScope,
        ContainerViewerShortcutScope,
    };
    Q_ENUM(ShortcutScope)

    explicit ImageActionAvailability(QObject *parent = nullptr);

    bool imageReady() const;
    void setImageReady(bool imageReady);
    bool fileDeletionInProgress() const;
    void setFileDeletionInProgress(bool fileDeletionInProgress);
    bool helpDialogOpen() const;
    void setHelpDialogOpen(bool helpDialogOpen);
    bool textInputFocused() const;
    void setTextInputFocused(bool textInputFocused);
    bool imagePannable() const;
    void setImagePannable(bool imagePannable);
    bool imageHorizontallyPannable() const;
    void setImageHorizontallyPannable(bool imageHorizontallyPannable);
    bool containerNavigationAvailable() const;
    void setContainerNavigationAvailable(bool containerNavigationAvailable);
    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool twoPageModeEnabled);
    bool twoPageModeAvailable() const;
    void setTwoPageModeAvailable(bool twoPageModeAvailable);
    bool rightToLeftReadingEnabled() const;
    void setRightToLeftReadingEnabled(bool rightToLeftReadingEnabled);
    bool rightToLeftReadingAvailable() const;
    void setRightToLeftReadingAvailable(bool rightToLeftReadingAvailable);
    bool scanBackwardAtFirstImageBoundary() const;
    void setScanBackwardAtFirstImageBoundary(bool scanBackwardAtFirstImageBoundary);

    bool canUseReadyActions() const;
    bool canUseRotateActions() const;
    bool canUseTwoPageModeActions() const;
    bool canUseRightToLeftReadingActions() const;
    bool rightToLeftReadingActive() const;
    bool twoPageModeActive() const;
    bool helpShortcutsEnabled() const;
    bool viewerShortcutsEnabled() const;
    bool readyShortcutsEnabled() const;
    bool readyViewerShortcutsEnabled() const;
    bool twoPageViewerShortcutsEnabled() const;
    bool rightToLeftReadingShortcutsEnabled() const;
    bool rightToLeftReadingViewerShortcutsEnabled() const;
    bool rotateShortcutsEnabled() const;
    bool rotateViewerShortcutsEnabled() const;
    bool pannableShortcutsEnabled() const;
    bool pannableViewerShortcutsEnabled() const;
    bool containerShortcutsEnabled() const;
    bool containerViewerShortcutsEnabled() const;
    int availabilityRevision() const;

    Q_INVOKABLE bool shortcutsEnabledForScope(ImageActionAvailability::ShortcutScope scope) const;

Q_SIGNALS:
    void availabilityChanged();

private:
    KiriView::ApplicationActions::ImageActionAvailabilityRuntime m_runtime;
};

static_assert(static_cast<int>(ImageActionAvailability::ContainerViewerShortcutScope)
    == static_cast<int>(
        KiriView::ApplicationActions::ImageShortcutScope::ContainerViewerShortcutScope));

#endif
