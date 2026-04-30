// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "avifcompat.h"

#include <QByteArray>
#include <algorithm>
#include <limits>
#include <optional>
#include <vector>

namespace {
constexpr qsizetype boxHeaderSize = 8;
constexpr qsizetype largeBoxExtraSize = 8;
constexpr qsizetype fullBoxHeaderSize = 4;
constexpr qsizetype ipmaEntryCountSize = 4;
constexpr qsizetype emptyIpmaBoxSize = 16;

struct Box {
    qsizetype offset = 0;
    qsizetype size = 0;
    qsizetype headerSize = 0;
    QByteArray type;

    qsizetype bodyOffset() const { return offset + headerSize; }
    qsizetype endOffset() const { return offset + size; }
};

struct IpmaBox {
    Box box;
    QByteArray versionAndFlags;
    quint32 entryCount = 0;
    QByteArray entries;
};

quint32 readUint32(const QByteArray &data, qsizetype offset)
{
    return (quint32(static_cast<unsigned char>(data.at(offset))) << 24)
        | (quint32(static_cast<unsigned char>(data.at(offset + 1))) << 16)
        | (quint32(static_cast<unsigned char>(data.at(offset + 2))) << 8)
        | quint32(static_cast<unsigned char>(data.at(offset + 3)));
}

quint64 readUint64(const QByteArray &data, qsizetype offset)
{
    return (quint64(readUint32(data, offset)) << 32) | readUint32(data, offset + 4);
}

quint16 readUint16(const QByteArray &data, qsizetype offset)
{
    return (quint16(static_cast<unsigned char>(data.at(offset))) << 8)
        | quint16(static_cast<unsigned char>(data.at(offset + 1)));
}

void writeUint32(QByteArray *data, qsizetype offset, quint32 value)
{
    (*data)[offset] = static_cast<char>((value >> 24) & 0xff);
    (*data)[offset + 1] = static_cast<char>((value >> 16) & 0xff);
    (*data)[offset + 2] = static_cast<char>((value >> 8) & 0xff);
    (*data)[offset + 3] = static_cast<char>(value & 0xff);
}

void writeUint16(QByteArray *data, qsizetype offset, quint16 value)
{
    (*data)[offset] = static_cast<char>((value >> 8) & 0xff);
    (*data)[offset + 1] = static_cast<char>(value & 0xff);
}

bool typeIs(const QByteArray &type, const char *expected)
{
    return type.size() == 4 && std::equal(type.cbegin(), type.cend(), expected);
}

std::optional<Box> readBox(const QByteArray &data, qsizetype offset, qsizetype endOffset)
{
    if (offset < 0 || endOffset < offset || endOffset - offset < boxHeaderSize) {
        return std::nullopt;
    }

    const quint32 smallSize = readUint32(data, offset);
    const QByteArray type = data.mid(offset + 4, 4);
    qsizetype headerSize = boxHeaderSize;
    quint64 size = smallSize;

    if (smallSize == 1) {
        if (endOffset - offset < boxHeaderSize + largeBoxExtraSize) {
            return std::nullopt;
        }
        size = readUint64(data, offset + boxHeaderSize);
        headerSize += largeBoxExtraSize;
    } else if (smallSize == 0) {
        size = static_cast<quint64>(endOffset - offset);
    }

    if (size < static_cast<quint64>(headerSize)
        || size > static_cast<quint64>(std::numeric_limits<qsizetype>::max())) {
        return std::nullopt;
    }

    const auto signedSize = static_cast<qsizetype>(size);
    if (signedSize > endOffset - offset) {
        return std::nullopt;
    }

    return Box { offset, signedSize, headerSize, type };
}

std::vector<Box> childBoxes(const QByteArray &data, qsizetype offset, qsizetype endOffset)
{
    std::vector<Box> boxes;

    while (offset < endOffset) {
        const std::optional<Box> box = readBox(data, offset, endOffset);
        if (!box.has_value()) {
            return {};
        }

        boxes.push_back(*box);
        offset = box->endOffset();
    }

    return boxes;
}

std::vector<Box> topLevelBoxes(const QByteArray &data) { return childBoxes(data, 0, data.size()); }

bool hasAvifBrand(const QByteArray &data, const Box &ftypBox)
{
    if (!typeIs(ftypBox.type, "ftyp") || ftypBox.size < ftypBox.headerSize + 8) {
        return false;
    }

    for (qsizetype offset = ftypBox.bodyOffset(); offset + 4 <= ftypBox.endOffset(); offset += 4) {
        const QByteArray brand = data.mid(offset, 4);
        if (typeIs(brand, "avif") || typeIs(brand, "avis")) {
            return true;
        }
    }

    return false;
}

bool isAvifFile(const QByteArray &data)
{
    for (const Box &box : topLevelBoxes(data)) {
        if (hasAvifBrand(data, box)) {
            return true;
        }
    }

    return false;
}

std::vector<Box> metaChildBoxes(const QByteArray &data, const Box &metaBox)
{
    if (!typeIs(metaBox.type, "meta") || metaBox.size < metaBox.headerSize + fullBoxHeaderSize) {
        return {};
    }

    return childBoxes(data, metaBox.bodyOffset() + fullBoxHeaderSize, metaBox.endOffset());
}

std::optional<quint32> primaryItemId(const QByteArray &data, const Box &metaBox)
{
    for (const Box &box : metaChildBoxes(data, metaBox)) {
        if (!typeIs(box.type, "pitm") || box.size < box.headerSize + fullBoxHeaderSize + 2) {
            continue;
        }

        const qsizetype versionOffset = box.bodyOffset();
        const auto version = static_cast<unsigned char>(data.at(versionOffset));
        const qsizetype itemIdOffset = versionOffset + fullBoxHeaderSize;
        if (version == 0 && itemIdOffset + 2 <= box.endOffset()) {
            return readUint16(data, itemIdOffset);
        }
        if (version == 1 && itemIdOffset + 4 <= box.endOffset()) {
            return readUint32(data, itemIdOffset);
        }
    }

    return std::nullopt;
}

bool auxCContainsAlpha(const QByteArray &data, const Box &auxCBox)
{
    return data.mid(auxCBox.bodyOffset(), auxCBox.size - auxCBox.headerSize)
        .contains(QByteArrayLiteral("alpha"));
}

bool hasAlphaAuxiliaryProperty(const QByteArray &data, const Box &metaBox)
{
    for (const Box &box : metaChildBoxes(data, metaBox)) {
        if (!typeIs(box.type, "iprp")) {
            continue;
        }

        for (const Box &iprpChild : childBoxes(data, box.bodyOffset(), box.endOffset())) {
            if (!typeIs(iprpChild.type, "ipco")) {
                continue;
            }

            for (const Box &propertyBox :
                childBoxes(data, iprpChild.bodyOffset(), iprpChild.endOffset())) {
                if (typeIs(propertyBox.type, "auxC") && auxCContainsAlpha(data, propertyBox)) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool patchItemReference(QByteArray *data, const Box &irefBox, quint32 itemId)
{
    if (irefBox.size < irefBox.headerSize + fullBoxHeaderSize) {
        return false;
    }

    bool changed = false;
    const auto version = static_cast<unsigned char>(data->at(irefBox.bodyOffset()));
    const qsizetype idSize = version == 1 ? 4 : 2;
    qsizetype offset = irefBox.bodyOffset() + fullBoxHeaderSize;

    while (offset < irefBox.endOffset()) {
        const std::optional<Box> referenceBox = readBox(*data, offset, irefBox.endOffset());
        if (!referenceBox.has_value()) {
            return changed;
        }

        qsizetype cursor = referenceBox->bodyOffset();
        if (referenceBox->endOffset() - cursor < idSize + 2) {
            return changed;
        }

        cursor += idSize;
        const quint16 referenceCount = readUint16(*data, cursor);
        cursor += 2;

        if (typeIs(referenceBox->type, "auxl")) {
            for (quint16 index = 0; index < referenceCount; ++index) {
                if (cursor + idSize > referenceBox->endOffset()) {
                    return changed;
                }

                const quint32 referencedItemId
                    = idSize == 4 ? readUint32(*data, cursor) : readUint16(*data, cursor);
                if (referencedItemId == 0) {
                    if (idSize == 4) {
                        writeUint32(data, cursor, itemId);
                    } else if (itemId <= std::numeric_limits<quint16>::max()) {
                        writeUint16(data, cursor, static_cast<quint16>(itemId));
                    } else {
                        return changed;
                    }
                    changed = true;
                }

                cursor += idSize;
            }
        }

        offset = referenceBox->endOffset();
    }

    return changed;
}

bool patchZeroAuxlReferences(QByteArray *data, const Box &metaBox)
{
    const std::optional<quint32> itemId = primaryItemId(*data, metaBox);
    if (!itemId.has_value() || *itemId == 0) {
        return false;
    }

    bool changed = false;
    for (const Box &box : metaChildBoxes(*data, metaBox)) {
        if (typeIs(box.type, "iref")) {
            changed = patchItemReference(data, box, *itemId) || changed;
        }
    }

    return changed;
}

std::optional<IpmaBox> readIpmaBox(const QByteArray &data, const Box &box)
{
    if (!typeIs(box.type, "ipma")
        || box.size < box.headerSize + fullBoxHeaderSize + ipmaEntryCountSize) {
        return std::nullopt;
    }

    const qsizetype bodyOffset = box.bodyOffset();
    IpmaBox ipma;
    ipma.box = box;
    ipma.versionAndFlags = data.mid(bodyOffset, fullBoxHeaderSize);
    ipma.entryCount = readUint32(data, bodyOffset + fullBoxHeaderSize);
    ipma.entries = data.mid(bodyOffset + fullBoxHeaderSize + ipmaEntryCountSize,
        box.endOffset() - bodyOffset - fullBoxHeaderSize - ipmaEntryCountSize);
    return ipma;
}

QByteArray emptyIpmaBoxWithAlternateVersion(const QByteArray &versionAndFlags)
{
    QByteArray box(emptyIpmaBoxSize, '\0');
    writeUint32(&box, 0, emptyIpmaBoxSize);
    box.replace(4, 4, QByteArrayLiteral("ipma"));
    box[8] = versionAndFlags.at(0) == '\0' ? '\1' : '\0';
    box[9] = versionAndFlags.at(1);
    box[10] = versionAndFlags.at(2);
    box[11] = versionAndFlags.at(3);
    return box;
}

bool patchAdjacentDuplicateIpmaBoxes(QByteArray *data, const IpmaBox &first, const IpmaBox &second)
{
    if (first.versionAndFlags != second.versionAndFlags
        || first.box.endOffset() != second.box.offset) {
        return false;
    }

    const QByteArray entries = first.entries + second.entries;
    const qsizetype mergedSize
        = boxHeaderSize + fullBoxHeaderSize + ipmaEntryCountSize + entries.size();
    const qsizetype availableSize = first.box.size + second.box.size;
    const qsizetype remainingSize = availableSize - mergedSize;

    if (mergedSize <= 0 || remainingSize != emptyIpmaBoxSize
        || first.entryCount
            > std::numeric_limits<quint32>::max() - static_cast<quint64>(second.entryCount)) {
        return false;
    }

    QByteArray replacement;
    replacement.reserve(availableSize);
    replacement.resize(mergedSize);
    writeUint32(&replacement, 0, static_cast<quint32>(mergedSize));
    replacement.replace(4, 4, QByteArrayLiteral("ipma"));
    replacement.replace(8, fullBoxHeaderSize, first.versionAndFlags);
    writeUint32(&replacement, 12, first.entryCount + second.entryCount);
    replacement.replace(16, entries.size(), entries);
    replacement.append(emptyIpmaBoxWithAlternateVersion(first.versionAndFlags));

    if (replacement.size() != availableSize) {
        return false;
    }

    data->replace(first.box.offset, availableSize, replacement);
    return true;
}

bool patchDuplicateIpmaBoxes(QByteArray *data, const Box &metaBox)
{
    bool changed = false;

    for (const Box &box : metaChildBoxes(*data, metaBox)) {
        if (!typeIs(box.type, "iprp")) {
            continue;
        }

        std::optional<IpmaBox> previousIpma;
        for (const Box &iprpChild : childBoxes(*data, box.bodyOffset(), box.endOffset())) {
            const std::optional<IpmaBox> ipma = readIpmaBox(*data, iprpChild);
            if (!ipma.has_value()) {
                previousIpma.reset();
                continue;
            }

            if (previousIpma.has_value()
                && patchAdjacentDuplicateIpmaBoxes(data, *previousIpma, *ipma)) {
                changed = true;
                previousIpma.reset();
                continue;
            }

            previousIpma = ipma;
        }
    }

    return changed;
}
}

namespace KiriView {
QByteArray avifDataWithCompatibilityFixes(const QByteArray &data)
{
    // HACK: AVIF alpha metadata compatibility until kf.imageformats supports it. 2026-04-30
    if (!isAvifFile(data)) {
        return data;
    }

    QByteArray fixedData;
    bool changed = false;

    for (const Box &box : topLevelBoxes(data)) {
        if (!typeIs(box.type, "meta") || !hasAlphaAuxiliaryProperty(data, box)) {
            continue;
        }

        if (fixedData.isEmpty()) {
            fixedData = data;
        }

        changed = patchZeroAuxlReferences(&fixedData, box) || changed;
        changed = patchDuplicateIpmaBoxes(&fixedData, box) || changed;
    }

    return changed ? fixedData : data;
}
}
