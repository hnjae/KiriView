// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONGEOMETRY_H
#define KIRIVIEW_IMAGEPRESENTATIONGEOMETRY_H

#include <QSize>

namespace KiriView {
class ImagePresentationGeometry final
{
public:
    const QSize &sourceImageSize() const;
    QSize logicalImageSize() const;
    int rotationDegrees() const;

    bool setSourceImageSize(const QSize &sourceImageSize);
    bool setRotationDegrees(int rotationDegrees);
    bool resetRotation();
    bool rotateClockwise();
    bool rotateCounterclockwise();

private:
    QSize m_sourceImageSize;
    int m_rotationDegrees = 0;
};
}

#endif
