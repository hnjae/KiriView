// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESHORTCUTNAVIGATIONPOLICY_H
#define KIRIVIEW_IMAGESHORTCUTNAVIGATIONPOLICY_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

class ImageShortcutNavigationPolicy : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum HorizontalArrowAction {
        PanLeft = 0,
        PanRight,
        OpenPreviousImage,
        OpenNextImage,
    };
    Q_ENUM(HorizontalArrowAction)

    enum SinglePageArrowAction {
        OpenPreviousSinglePage = 0,
        OpenNextSinglePage,
    };
    Q_ENUM(SinglePageArrowAction)

    enum ScanAction {
        NoScanAction = 0,
        OpenPreviousImageFromScan,
        OpenNextImageFromScan,
        OpenPreviousPageFromFinalScanStart,
        ShowFirstImageBoundary,
    };
    Q_ENUM(ScanAction)

    explicit ImageShortcutNavigationPolicy(QObject *parent = nullptr);

    Q_INVOKABLE ImageShortcutNavigationPolicy::HorizontalArrowAction horizontalArrowAction(
        bool leftArrow, bool horizontallyPannable, bool rightToLeftReadingActive) const;
    Q_INVOKABLE ImageShortcutNavigationPolicy::SinglePageArrowAction singlePageArrowAction(
        bool leftArrow, bool rightToLeftReadingActive) const;
    Q_INVOKABLE ImageShortcutNavigationPolicy::ScanAction scanForwardAction(
        bool imagePannable, bool viewportMoved) const;
    Q_INVOKABLE ImageShortcutNavigationPolicy::ScanAction scanBackwardAction(
        bool imagePannable, bool viewportMoved, bool atFirstImage, int currentPageNumber) const;
};

#endif
