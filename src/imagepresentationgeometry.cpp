// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationgeometry.h"

#include "imagerotation.h"

namespace KiriView {
const QSize &ImagePresentationGeometry::sourceImageSize() const { return m_sourceImageSize; }

QSize ImagePresentationGeometry::logicalImageSize() const
{
    return rotatedImageSize(m_sourceImageSize, m_rotationDegrees);
}

int ImagePresentationGeometry::rotationDegrees() const { return m_rotationDegrees; }

bool ImagePresentationGeometry::setSourceImageSize(const QSize &sourceImageSize)
{
    if (m_sourceImageSize == sourceImageSize) {
        return false;
    }

    m_sourceImageSize = sourceImageSize;
    return true;
}

bool ImagePresentationGeometry::setRotationDegrees(int rotationDegrees)
{
    const int normalizedRotationDegrees = normalizedImageRotationDegrees(rotationDegrees);
    if (m_rotationDegrees == normalizedRotationDegrees) {
        return false;
    }

    m_rotationDegrees = normalizedRotationDegrees;
    return true;
}

bool ImagePresentationGeometry::resetRotation() { return setRotationDegrees(0); }

bool ImagePresentationGeometry::rotateClockwise()
{
    return setRotationDegrees(imageRotationClockwise(m_rotationDegrees));
}

bool ImagePresentationGeometry::rotateCounterclockwise()
{
    return setRotationDegrees(imageRotationCounterclockwise(m_rotationDegrees));
}
}
