// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::byteio::read_be_u32;
use crate::fileextension::extension_for_file_name;

pub(crate) const RAW_IMAGE_EXTENSIONS: &[&str] = &[
    "3fr", "arw", "bay", "bmq", "cr2", "cr3", "crw", "cs1", "cs2", "dcr", "dng", "erf", "fff",
    "iiq", "k25", "kdc", "mdc", "mef", "mos", "mrw", "nef", "nrw", "orf", "pef", "raf", "raw",
    "rdc", "rwl", "rw2", "sr2", "srf", "srw", "x3f",
];

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageInputKind {
        Unknown = 0,
        Svg = 1,
        Apng = 2,
        HeifFamily = 3,
        Raw = 4,
        QtRaster = 5,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustQtRasterFormat {
        None = 0,
        Png = 1,
        Jpeg = 2,
        Gif = 3,
        Webp = 4,
        Bmp = 5,
        Tiff = 6,
        Jxl = 7,
        Jp2 = 8,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDecodeDataSource {
        Original = 0,
        AvifCompatible = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageInputClassification {
        kind: RustImageInputKind,
        qt_format: RustQtRasterFormat,
        data_source: RustImageDecodeDataSource,
    }

    extern "Rust" {
        #[cxx_name = "rustClassifyImageInput"]
        fn rust_classify_image_input(data: &[u8], file_name: &str) -> RustImageInputClassification;
    }
}

use ffi::{
    RustImageDecodeDataSource, RustImageInputClassification, RustImageInputKind, RustQtRasterFormat,
};

const PNG_SIGNATURE: &[u8] = b"\x89PNG\r\n\x1a\n";
const JPEG2000_SIGNATURE: &[u8] = b"\0\0\0\x0cjP  \r\n\x87\n";
const JXL_CODESTREAM_SIGNATURE: &[u8] = b"\xff\x0a";
const JXL_CONTAINER_SIGNATURE: &[u8] = b"\0\0\0\x0cJXL \r\n\x87\n";
const FTYP_BOX_TYPE_OFFSET: usize = 4;
const FTYP_MAJOR_BRAND_OFFSET: usize = 8;
const FTYP_COMPATIBLE_BRANDS_OFFSET: usize = 16;
const BRAND_SIZE: usize = 4;
const TIFF_PHOTOMETRIC_INTERPRETATION_TAG: u16 = 262;
const TIFF_DNG_VERSION_TAG: u16 = 50706;
const TIFF_SHORT_TYPE: u16 = 3;
const TIFF_CFA_PHOTOMETRIC_INTERPRETATION: u16 = 32803;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum TiffByteOrder {
    LittleEndian,
    BigEndian,
}

fn rust_classify_image_input(data: &[u8], file_name: &str) -> RustImageInputClassification {
    classify_image_input(data, file_name)
}

fn classify_image_input(data: &[u8], file_name: &str) -> RustImageInputClassification {
    if data.starts_with(PNG_SIGNATURE) {
        if png_contains_animation_control(data) {
            return classification(RustImageInputKind::Apng);
        }
        return qt_raster(RustQtRasterFormat::Png);
    }
    if data.starts_with(b"\xff\xd8\xff") {
        return qt_raster(RustQtRasterFormat::Jpeg);
    }
    if data.starts_with(b"GIF87a") || data.starts_with(b"GIF89a") {
        return qt_raster(RustQtRasterFormat::Gif);
    }
    if data.len() >= 12 && data.starts_with(b"RIFF") && data.get(8..12) == Some(b"WEBP") {
        return qt_raster(RustQtRasterFormat::Webp);
    }
    if data.starts_with(b"BM") {
        return qt_raster(RustQtRasterFormat::Bmp);
    }
    if data.starts_with(JXL_CODESTREAM_SIGNATURE) || data.starts_with(JXL_CONTAINER_SIGNATURE) {
        return qt_raster(RustQtRasterFormat::Jxl);
    }
    if data.starts_with(JPEG2000_SIGNATURE) {
        return qt_raster(RustQtRasterFormat::Jp2);
    }
    if is_tiff_family(data) {
        if file_name_has_raw_extension(file_name) || is_likely_tiff_raw_image_data(data) {
            return classification(RustImageInputKind::Raw);
        }
        return qt_raster(RustQtRasterFormat::Tiff);
    }
    if let Some(classification) = bmff_classification(data) {
        return classification;
    }
    if file_name_has_svg_extension(file_name) || looks_like_svg(data) {
        return classification(RustImageInputKind::Svg);
    }
    if file_name_has_raw_extension(file_name) {
        return classification(RustImageInputKind::Raw);
    }

    unknown()
}

fn classification(kind: RustImageInputKind) -> RustImageInputClassification {
    RustImageInputClassification {
        kind,
        qt_format: RustQtRasterFormat::None,
        data_source: data_source_for_kind(kind),
    }
}

fn qt_raster(format: RustQtRasterFormat) -> RustImageInputClassification {
    RustImageInputClassification {
        kind: RustImageInputKind::QtRaster,
        qt_format: format,
        data_source: RustImageDecodeDataSource::Original,
    }
}

fn unknown() -> RustImageInputClassification {
    classification(RustImageInputKind::Unknown)
}

fn data_source_for_kind(kind: RustImageInputKind) -> RustImageDecodeDataSource {
    match kind {
        RustImageInputKind::HeifFamily => RustImageDecodeDataSource::AvifCompatible,
        RustImageInputKind::Unknown
        | RustImageInputKind::Svg
        | RustImageInputKind::Apng
        | RustImageInputKind::Raw
        | RustImageInputKind::QtRaster => RustImageDecodeDataSource::Original,
        _ => RustImageDecodeDataSource::Original,
    }
}

fn png_contains_animation_control(data: &[u8]) -> bool {
    let mut offset = PNG_SIGNATURE.len();
    while offset.checked_add(12).is_some_and(|end| end <= data.len()) {
        let Some(length) = read_be_u32(data, offset).and_then(|value| usize::try_from(value).ok())
        else {
            return false;
        };
        let chunk_type_offset = offset + 4;
        let Some(chunk_type) = data.get(chunk_type_offset..chunk_type_offset + 4) else {
            return false;
        };
        if chunk_type == b"acTL" {
            return true;
        }
        if chunk_type == b"IDAT" || chunk_type == b"IEND" {
            return false;
        }
        let Some(next_offset) = offset
            .checked_add(12)
            .and_then(|header| header.checked_add(length))
        else {
            return false;
        };
        if next_offset > data.len() {
            return false;
        }
        offset = next_offset;
    }
    false
}

fn bmff_classification(data: &[u8]) -> Option<RustImageInputClassification> {
    if data.len() < FTYP_COMPATIBLE_BRANDS_OFFSET
        || data.get(FTYP_BOX_TYPE_OFFSET..FTYP_BOX_TYPE_OFFSET + BRAND_SIZE) != Some(b"ftyp")
    {
        return None;
    }

    let box_size = usize::try_from(read_be_u32(data, 0)?).ok()?;
    if !(FTYP_COMPATIBLE_BRANDS_OFFSET..=data.len()).contains(&box_size) {
        return None;
    }

    let mut raw = false;
    let mut heif_family = false;
    let mut jxl = false;
    let mut jp2 = false;
    let mut record_brand = |brand: &[u8]| {
        if is_cr3_brand(brand) {
            raw = true;
        } else if is_heif_family_brand(brand) {
            heif_family = true;
        } else if brand == b"jxl " {
            jxl = true;
        } else if is_jpeg2000_brand(brand) {
            jp2 = true;
        }
    };

    record_brand(&data[FTYP_MAJOR_BRAND_OFFSET..FTYP_MAJOR_BRAND_OFFSET + BRAND_SIZE]);
    let mut offset = FTYP_COMPATIBLE_BRANDS_OFFSET;
    while offset + BRAND_SIZE <= box_size {
        record_brand(&data[offset..offset + BRAND_SIZE]);
        offset += BRAND_SIZE;
    }

    if raw {
        return Some(classification(RustImageInputKind::Raw));
    }
    if heif_family {
        return Some(classification(RustImageInputKind::HeifFamily));
    }
    if jxl {
        return Some(qt_raster(RustQtRasterFormat::Jxl));
    }
    if jp2 {
        return Some(qt_raster(RustQtRasterFormat::Jp2));
    }

    None
}

fn is_heif_family_brand(brand: &[u8]) -> bool {
    matches!(
        brand,
        b"avci"
            | b"avcs"
            | b"avif"
            | b"avis"
            | b"heic"
            | b"heif"
            | b"heim"
            | b"heis"
            | b"heix"
            | b"hevc"
            | b"hevm"
            | b"hevs"
            | b"hevx"
            | b"hej2"
            | b"j2is"
            | b"j2ki"
            | b"jpeg"
            | b"jpgs"
            | b"mif1"
            | b"mif2"
            | b"msf1"
            | b"vvic"
            | b"vvis"
    )
}

fn is_cr3_brand(brand: &[u8]) -> bool {
    matches!(brand, b"crx " | b"cr3 ")
}

fn is_jpeg2000_brand(brand: &[u8]) -> bool {
    matches!(brand, b"jp2 " | b"jpx " | b"jpm ")
}

fn is_tiff_family(data: &[u8]) -> bool {
    tiff_byte_order(data).is_some()
}

fn tiff_byte_order(data: &[u8]) -> Option<TiffByteOrder> {
    match data.get(0..4)? {
        [b'I', b'I', 42, 0] => Some(TiffByteOrder::LittleEndian),
        [b'M', b'M', 0, 42] => Some(TiffByteOrder::BigEndian),
        _ => None,
    }
}

fn tiff_u16(data: &[u8], offset: usize, byte_order: TiffByteOrder) -> Option<u16> {
    let bytes: [u8; 2] = data.get(offset..offset.checked_add(2)?)?.try_into().ok()?;
    Some(match byte_order {
        TiffByteOrder::LittleEndian => u16::from_le_bytes(bytes),
        TiffByteOrder::BigEndian => u16::from_be_bytes(bytes),
    })
}

fn tiff_u32(data: &[u8], offset: usize, byte_order: TiffByteOrder) -> Option<u32> {
    let bytes: [u8; 4] = data.get(offset..offset.checked_add(4)?)?.try_into().ok()?;
    Some(match byte_order {
        TiffByteOrder::LittleEndian => u32::from_le_bytes(bytes),
        TiffByteOrder::BigEndian => u32::from_be_bytes(bytes),
    })
}

fn tiff_short_value(
    data: &[u8],
    tag_entry_offset: usize,
    value_count: u32,
    byte_order: TiffByteOrder,
) -> Option<u16> {
    if value_count == 0 {
        return None;
    }

    if value_count <= 2 {
        return tiff_u16(data, tag_entry_offset + 8, byte_order);
    }

    let value_offset = usize::try_from(tiff_u32(data, tag_entry_offset + 8, byte_order)?).ok()?;
    tiff_u16(data, value_offset, byte_order)
}

fn is_likely_tiff_raw_image_data(data: &[u8]) -> bool {
    let Some(byte_order) = tiff_byte_order(data) else {
        return false;
    };

    if contains_subslice(
        data,
        &[0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0x80],
    ) || contains_subslice(
        data,
        &[0x01, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x80, 0x23],
    ) || contains_subslice(data, &[0x12, 0xc6, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00])
        || contains_subslice(data, &[0xc6, 0x12, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04])
    {
        return true;
    }

    let Some(ifd_offset) =
        tiff_u32(data, 4, byte_order).and_then(|value| usize::try_from(value).ok())
    else {
        return false;
    };
    let Some(tag_count) = tiff_u16(data, ifd_offset, byte_order) else {
        return false;
    };
    let Some(tag_entries_offset) = ifd_offset.checked_add(2) else {
        return false;
    };
    let Some(tag_entries_size) = usize::from(tag_count).checked_mul(12) else {
        return false;
    };
    if tag_entries_offset
        .checked_add(tag_entries_size)
        .is_none_or(|end| end > data.len())
    {
        return false;
    }

    for tag_index in 0..usize::from(tag_count) {
        let tag_entry_offset = tag_entries_offset + tag_index * 12;
        let Some(tag) = tiff_u16(data, tag_entry_offset, byte_order) else {
            continue;
        };
        if tag == TIFF_DNG_VERSION_TAG {
            return true;
        }
        if tag != TIFF_PHOTOMETRIC_INTERPRETATION_TAG {
            continue;
        }

        let Some(field_type) = tiff_u16(data, tag_entry_offset + 2, byte_order) else {
            continue;
        };
        let Some(value_count) = tiff_u32(data, tag_entry_offset + 4, byte_order) else {
            continue;
        };
        if field_type != TIFF_SHORT_TYPE {
            continue;
        }

        if tiff_short_value(data, tag_entry_offset, value_count, byte_order)
            == Some(TIFF_CFA_PHOTOMETRIC_INTERPRETATION)
        {
            return true;
        }
    }

    false
}

fn contains_subslice(data: &[u8], needle: &[u8]) -> bool {
    !needle.is_empty() && data.windows(needle.len()).any(|window| window == needle)
}

fn looks_like_svg(data: &[u8]) -> bool {
    let scan_len = data.len().min(4096);
    let mut slice = &data[..scan_len];
    if slice.starts_with(b"\xef\xbb\xbf") {
        slice = &slice[3..];
    }
    slice = trim_ascii_whitespace_start(slice);
    if slice.starts_with(b"<svg") || slice.starts_with(b"<SVG") {
        return true;
    }

    let lowercase = slice.to_ascii_lowercase();
    if lowercase.starts_with(b"<?xml")
        || lowercase.starts_with(b"<!--")
        || lowercase.starts_with(b"<!doctype")
    {
        return lowercase
            .windows(b"<svg".len())
            .any(|window| window == b"<svg");
    }
    false
}

fn trim_ascii_whitespace_start(data: &[u8]) -> &[u8] {
    let Some(first_non_whitespace) = data.iter().position(|byte| !byte.is_ascii_whitespace())
    else {
        return &[];
    };
    &data[first_non_whitespace..]
}

fn file_name_has_svg_extension(name: &str) -> bool {
    extension_for_file_name(name).is_some_and(|extension| extension == "svg")
}

fn file_name_has_raw_extension(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| RAW_IMAGE_EXTENSIONS.contains(&extension.as_str()))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn png_chunk(kind: &[u8; 4], body: &[u8]) -> Vec<u8> {
        let mut chunk = Vec::new();
        chunk.extend_from_slice(&(body.len() as u32).to_be_bytes());
        chunk.extend_from_slice(kind);
        chunk.extend_from_slice(body);
        chunk.extend_from_slice(&[0, 0, 0, 0]);
        chunk
    }

    fn png_data(chunks: &[Vec<u8>]) -> Vec<u8> {
        let mut data = Vec::new();
        data.extend_from_slice(PNG_SIGNATURE);
        for chunk in chunks {
            data.extend_from_slice(chunk);
        }
        data
    }

    fn ftyp_box(major_brand: &[u8; 4], compatible_brands: &[[u8; 4]]) -> Vec<u8> {
        let box_size = FTYP_COMPATIBLE_BRANDS_OFFSET + compatible_brands.len() * BRAND_SIZE;
        let mut data = Vec::new();
        data.extend_from_slice(&(box_size as u32).to_be_bytes());
        data.extend_from_slice(b"ftyp");
        data.extend_from_slice(major_brand);
        data.extend_from_slice(&0u32.to_be_bytes());
        for brand in compatible_brands {
            data.extend_from_slice(brand);
        }
        data
    }

    fn ordinary_tiff_data() -> Vec<u8> {
        let mut data = Vec::new();
        data.extend_from_slice(b"II*\0");
        data.extend_from_slice(&8u32.to_le_bytes());
        data.extend_from_slice(&1u16.to_le_bytes());
        data.extend_from_slice(&TIFF_PHOTOMETRIC_INTERPRETATION_TAG.to_le_bytes());
        data.extend_from_slice(&TIFF_SHORT_TYPE.to_le_bytes());
        data.extend_from_slice(&1u32.to_le_bytes());
        data.extend_from_slice(&2u16.to_le_bytes());
        data.extend_from_slice(&0u16.to_le_bytes());
        data.extend_from_slice(&0u32.to_le_bytes());
        data
    }

    #[test]
    fn classifies_png_and_apng_from_content() {
        let png = png_data(&[png_chunk(b"IHDR", &[0; 13]), png_chunk(b"IDAT", &[0; 1])]);
        assert_eq!(
            classify_image_input(&png, "image.raw"),
            qt_raster(RustQtRasterFormat::Png)
        );

        let apng = png_data(&[
            png_chunk(b"IHDR", &[0; 13]),
            png_chunk(b"acTL", &[0; 8]),
            png_chunk(b"IDAT", &[0; 1]),
        ]);
        assert_eq!(
            classify_image_input(&apng, "image.png").kind,
            RustImageInputKind::Apng
        );
    }

    #[test]
    fn classifies_common_raster_magic_bytes() {
        assert_eq!(
            classify_image_input(b"\xff\xd8\xff rest", "photo.bin"),
            qt_raster(RustQtRasterFormat::Jpeg)
        );
        assert_eq!(
            classify_image_input(b"GIF89a rest", "image.bin"),
            qt_raster(RustQtRasterFormat::Gif)
        );
        assert_eq!(
            classify_image_input(b"RIFF\0\0\0\0WEBP rest", "image.bin"),
            qt_raster(RustQtRasterFormat::Webp)
        );
        assert_eq!(
            classify_image_input(b"BM rest", "image.bin"),
            qt_raster(RustQtRasterFormat::Bmp)
        );
        assert_eq!(
            classify_image_input(JXL_CODESTREAM_SIGNATURE, "image.bin"),
            qt_raster(RustQtRasterFormat::Jxl)
        );
        assert_eq!(
            classify_image_input(JPEG2000_SIGNATURE, "image.bin"),
            qt_raster(RustQtRasterFormat::Jp2)
        );
    }

    #[test]
    fn classifies_tiff_family_as_raw_or_qt_tiff() {
        let tiff = ordinary_tiff_data();
        assert_eq!(
            classify_image_input(&tiff, "scan.tiff"),
            qt_raster(RustQtRasterFormat::Tiff)
        );
        assert_eq!(
            classify_image_input(&tiff, "scan.dng").kind,
            RustImageInputKind::Raw
        );

        let raw_dng = include_bytes!("../../tests/fixtures/raw-cfa-smoke.dng");
        assert_eq!(
            classify_image_input(raw_dng, "raw-cfa-smoke").kind,
            RustImageInputKind::Raw
        );
    }

    #[test]
    fn classifies_bmff_families_by_brand() {
        assert_eq!(
            classify_image_input(&ftyp_box(b"avif", &[]), "image.bin").kind,
            RustImageInputKind::HeifFamily
        );
        assert_eq!(
            classify_image_input(&ftyp_box(b"zzzz", &[*b"heic"]), "image.bin").kind,
            RustImageInputKind::HeifFamily
        );
        assert_eq!(
            classify_image_input(&ftyp_box(b"crx ", &[]), "image.bin").kind,
            RustImageInputKind::Raw
        );
        assert_eq!(
            classify_image_input(&ftyp_box(b"jxl ", &[]), "image.bin"),
            qt_raster(RustQtRasterFormat::Jxl)
        );
        assert_eq!(
            classify_image_input(&ftyp_box(b"jp2 ", &[]), "image.bin"),
            qt_raster(RustQtRasterFormat::Jp2)
        );
    }

    #[test]
    fn classifies_svg_and_raw_extension_hints_after_magic_bytes() {
        assert_eq!(
            classify_image_input(b"<?xml version=\"1.0\"?><svg/>", "image.bin").kind,
            RustImageInputKind::Svg
        );
        assert_eq!(
            classify_image_input(b"unrecognized bytes", "photo.nef").kind,
            RustImageInputKind::Raw
        );
        assert_eq!(
            classify_image_input(b"unrecognized bytes", "image.svg").kind,
            RustImageInputKind::Svg
        );
    }

    #[test]
    fn unknown_bytes_do_not_become_qt_raster() {
        assert_eq!(
            classify_image_input(b"unrecognized bytes", "image.bin").kind,
            RustImageInputKind::Unknown
        );
    }
}
