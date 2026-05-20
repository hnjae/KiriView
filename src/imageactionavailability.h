// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEACTIONAVAILABILITY_H
#define KIRIVIEW_IMAGEACTIONAVAILABILITY_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

struct ImageActionAvailabilityInput {
    bool imageReady = false;
    int imageCount = 0;
    int currentPageNumber = 0;
    int currentLastPageNumber = 0;
    bool fileDeletionInProgress = false;
    bool helpDialogOpen = false;
    bool textInputFocused = false;
    bool imagePannable = false;
    bool imageHorizontallyPannable = false;
    bool containerNavigationAvailable = false;
    bool twoPageModeEnabled = false;
    bool twoPageModeAvailable = false;
    bool rightToLeftReadingEnabled = false;
    bool rightToLeftReadingAvailable = false;
};

struct ImageActionAvailabilityProjection {
    bool canOpenNextImage = false;
    bool canOpenPreviousImage = false;
    bool atKnownFirstImage = false;
    bool atKnownLastImage = false;
    bool canUsePageActions = false;
    bool canUseReadyActions = false;
    bool canUseRotateActions = false;
    bool canUseTwoPageModeActions = false;
    bool canUseRightToLeftReadingActions = false;
    bool rightToLeftReadingActive = false;
    bool twoPageModeActive = false;
    bool helpShortcutsEnabled = false;
    bool viewerShortcutsEnabled = false;
    bool readyShortcutsEnabled = false;
    bool readyViewerShortcutsEnabled = false;
    bool imageSelectionShortcutsEnabled = false;
    bool imageSelectionViewerShortcutsEnabled = false;
    bool pageShortcutsEnabled = false;
    bool pageViewerShortcutsEnabled = false;
    bool twoPageViewerShortcutsEnabled = false;
    bool rightToLeftReadingShortcutsEnabled = false;
    bool rightToLeftReadingViewerShortcutsEnabled = false;
    bool rotateShortcutsEnabled = false;
    bool rotateViewerShortcutsEnabled = false;
    bool pannableShortcutsEnabled = false;
    bool pannableViewerShortcutsEnabled = false;
    bool containerShortcutsEnabled = false;
    bool containerViewerShortcutsEnabled = false;
};

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const ImageActionAvailabilityInput &input);

class ImageActionAvailability : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool imageReady READ imageReady WRITE setImageReady NOTIFY availabilityChanged)
    Q_PROPERTY(int imageCount READ imageCount WRITE setImageCount NOTIFY availabilityChanged)
    Q_PROPERTY(int currentPageNumber READ currentPageNumber WRITE setCurrentPageNumber NOTIFY
            availabilityChanged)
    Q_PROPERTY(int currentLastPageNumber READ currentLastPageNumber WRITE setCurrentLastPageNumber
            NOTIFY availabilityChanged)
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

    Q_PROPERTY(bool canOpenNextImage READ canOpenNextImage NOTIFY availabilityChanged)
    Q_PROPERTY(bool canOpenPreviousImage READ canOpenPreviousImage NOTIFY availabilityChanged)
    Q_PROPERTY(bool atKnownFirstImage READ atKnownFirstImage NOTIFY availabilityChanged)
    Q_PROPERTY(bool atKnownLastImage READ atKnownLastImage NOTIFY availabilityChanged)
    Q_PROPERTY(bool canUsePageActions READ canUsePageActions NOTIFY availabilityChanged)
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
    Q_PROPERTY(bool imageSelectionShortcutsEnabled READ imageSelectionShortcutsEnabled NOTIFY
            availabilityChanged)
    Q_PROPERTY(bool imageSelectionViewerShortcutsEnabled READ imageSelectionViewerShortcutsEnabled
            NOTIFY availabilityChanged)
    Q_PROPERTY(bool pageShortcutsEnabled READ pageShortcutsEnabled NOTIFY availabilityChanged)
    Q_PROPERTY(
        bool pageViewerShortcutsEnabled READ pageViewerShortcutsEnabled NOTIFY availabilityChanged)
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
    int imageCount() const;
    void setImageCount(int imageCount);
    int currentPageNumber() const;
    void setCurrentPageNumber(int currentPageNumber);
    int currentLastPageNumber() const;
    void setCurrentLastPageNumber(int currentLastPageNumber);
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

    bool canOpenNextImage() const;
    bool canOpenPreviousImage() const;
    bool atKnownFirstImage() const;
    bool atKnownLastImage() const;
    bool canUsePageActions() const;
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
    bool imageSelectionShortcutsEnabled() const;
    bool imageSelectionViewerShortcutsEnabled() const;
    bool pageShortcutsEnabled() const;
    bool pageViewerShortcutsEnabled() const;
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

    Q_INVOKABLE bool shortcutsEnabledForScope(ShortcutScope scope) const;

Q_SIGNALS:
    void availabilityChanged();

private:
    void setBool(bool &target, bool value);
    void setInt(int &target, int value);
    void publishInputChange();

    ImageActionAvailabilityInput m_input;
    ImageActionAvailabilityProjection m_projection;
    int m_availabilityRevision = 0;
};

#endif
