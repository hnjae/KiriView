// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::byteio::read_be_u32;
use crate::heifbrands::{HeifBrandKind, heif_brand_kind as policy_heif_brand_kind};

#[cxx::bridge(namespace = "kiriview")]
mod ffi {
    enum RustHeifBrandKind {
        Unknown = 0,
        StillImage = 1,
        ImageSequence = 2,
    }

    struct RustHeifContainerInfo {
        still_image: bool,
        image_sequence: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustHeifBrandKind"]
        fn rust_heif_brand_kind(brand: &[u8]) -> RustHeifBrandKind;

        #[cxx_name = "rustHeifContainerInfo"]
        fn rust_heif_container_info(data: &[u8]) -> RustHeifContainerInfo;
    }
}

use ffi::{RustHeifBrandKind, RustHeifContainerInfo};

const FTYP_BOX_TYPE_OFFSET: usize = 4;
const FTYP_MAJOR_BRAND_OFFSET: usize = 8;
const FTYP_COMPATIBLE_BRANDS_OFFSET: usize = 16;
const BRAND_SIZE: usize = 4;

fn rust_heif_brand_kind(brand: &[u8]) -> RustHeifBrandKind {
    heif_brand_kind(brand)
}

fn rust_heif_container_info(data: &[u8]) -> RustHeifContainerInfo {
    scan_heif_brands(data)
}

fn heif_brand_kind(brand: &[u8]) -> RustHeifBrandKind {
    match policy_heif_brand_kind(brand) {
        HeifBrandKind::StillImage => RustHeifBrandKind::StillImage,
        HeifBrandKind::ImageSequence => RustHeifBrandKind::ImageSequence,
        HeifBrandKind::Unknown => RustHeifBrandKind::Unknown,
    }
}

fn scan_heif_brands(data: &[u8]) -> RustHeifContainerInfo {
    let Some(box_size) = ftyp_box_size(data) else {
        return empty_info();
    };

    let mut info = empty_info();
    record_brand(
        &mut info,
        &data[FTYP_MAJOR_BRAND_OFFSET..FTYP_MAJOR_BRAND_OFFSET + BRAND_SIZE],
    );

    let mut offset = FTYP_COMPATIBLE_BRANDS_OFFSET;
    while offset + BRAND_SIZE <= box_size {
        record_brand(&mut info, &data[offset..offset + BRAND_SIZE]);
        offset += BRAND_SIZE;
    }

    info
}

fn ftyp_box_size(data: &[u8]) -> Option<usize> {
    if data.len() < FTYP_COMPATIBLE_BRANDS_OFFSET
        || data.get(FTYP_BOX_TYPE_OFFSET..FTYP_BOX_TYPE_OFFSET + BRAND_SIZE) != Some(b"ftyp")
    {
        return None;
    }

    let box_size = usize::try_from(read_be_u32(data, 0)?).ok()?;
    if box_size < FTYP_COMPATIBLE_BRANDS_OFFSET || box_size > data.len() {
        return None;
    }

    Some(box_size)
}

fn record_brand(info: &mut RustHeifContainerInfo, brand: &[u8]) {
    match heif_brand_kind(brand) {
        RustHeifBrandKind::StillImage => info.still_image = true,
        RustHeifBrandKind::ImageSequence => info.image_sequence = true,
        RustHeifBrandKind::Unknown => {}
        _ => {}
    }
}

fn empty_info() -> RustHeifContainerInfo {
    RustHeifContainerInfo {
        still_image: false,
        image_sequence: false,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ftyp_box(major_brand: &[u8; 4], compatible_brands: &[[u8; 4]]) -> Vec<u8> {
        let box_size = u32::try_from(FTYP_COMPATIBLE_BRANDS_OFFSET + compatible_brands.len() * 4)
            .expect("test box should fit in u32");
        let mut data = Vec::new();
        data.extend_from_slice(&box_size.to_be_bytes());
        data.extend_from_slice(b"ftyp");
        data.extend_from_slice(major_brand);
        data.extend_from_slice(&0u32.to_be_bytes());
        for brand in compatible_brands {
            data.extend_from_slice(brand);
        }
        data
    }

    #[test]
    fn classifies_known_brands() {
        assert!(matches!(
            heif_brand_kind(b"avif"),
            RustHeifBrandKind::StillImage
        ));
        assert!(matches!(
            heif_brand_kind(b"avis"),
            RustHeifBrandKind::ImageSequence
        ));
        assert!(matches!(
            heif_brand_kind(b"png "),
            RustHeifBrandKind::Unknown
        ));
        assert!(matches!(
            heif_brand_kind(b"too long"),
            RustHeifBrandKind::Unknown
        ));
    }

    #[test]
    fn scans_major_and_compatible_brands() {
        let still = scan_heif_brands(&ftyp_box(b"avif", &[]));
        assert!(still.still_image);
        assert!(!still.image_sequence);

        let sequence = scan_heif_brands(&ftyp_box(b"zzzz", &[*b"avcs"]));
        assert!(!sequence.still_image);
        assert!(sequence.image_sequence);

        let mixed = scan_heif_brands(&ftyp_box(b"heic", &[*b"avis"]));
        assert!(mixed.still_image);
        assert!(mixed.image_sequence);
    }

    #[test]
    fn rejects_malformed_ftyp_boxes() {
        let mut truncated = ftyp_box(b"avif", &[]);
        truncated.truncate(15);
        assert!(!scan_heif_brands(&truncated).still_image);

        let mut wrong_type = ftyp_box(b"avif", &[]);
        wrong_type[4..8].copy_from_slice(b"free");
        assert!(!scan_heif_brands(&wrong_type).still_image);

        let mut undersized = ftyp_box(b"avif", &[]);
        undersized[0..4].copy_from_slice(&12u32.to_be_bytes());
        assert!(!scan_heif_brands(&undersized).still_image);

        let mut oversized = ftyp_box(b"avif", &[]);
        oversized[0..4].copy_from_slice(&24u32.to_be_bytes());
        assert!(!scan_heif_brands(&oversized).still_image);
    }

    #[test]
    fn ignores_partial_compatible_brand_tail() {
        let mut data = ftyp_box(b"zzzz", &[*b"avif"]);
        data.extend_from_slice(b"avi");
        let box_size = data.len() as u32;
        data[0..4].copy_from_slice(&box_size.to_be_bytes());

        let info = scan_heif_brands(&data);
        assert!(info.still_image);
        assert!(!info.image_sequence);
    }
}
