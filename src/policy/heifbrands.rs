// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum HeifBrandKind {
    Unknown,
    StillImage,
    ImageSequence,
}

const HEIF_BRAND_KINDS: &[([u8; 4], HeifBrandKind)] = &[
    (*b"avci", HeifBrandKind::StillImage),
    (*b"avif", HeifBrandKind::StillImage),
    (*b"heic", HeifBrandKind::StillImage),
    (*b"heif", HeifBrandKind::StillImage),
    (*b"heim", HeifBrandKind::StillImage),
    (*b"heis", HeifBrandKind::StillImage),
    (*b"heix", HeifBrandKind::StillImage),
    (*b"hej2", HeifBrandKind::StillImage),
    (*b"j2ki", HeifBrandKind::StillImage),
    (*b"jpeg", HeifBrandKind::StillImage),
    (*b"mif1", HeifBrandKind::StillImage),
    (*b"mif2", HeifBrandKind::StillImage),
    (*b"vvic", HeifBrandKind::StillImage),
    (*b"avcs", HeifBrandKind::ImageSequence),
    (*b"avis", HeifBrandKind::ImageSequence),
    (*b"hevc", HeifBrandKind::ImageSequence),
    (*b"hevm", HeifBrandKind::ImageSequence),
    (*b"hevs", HeifBrandKind::ImageSequence),
    (*b"hevx", HeifBrandKind::ImageSequence),
    (*b"j2is", HeifBrandKind::ImageSequence),
    (*b"jpgs", HeifBrandKind::ImageSequence),
    (*b"msf1", HeifBrandKind::ImageSequence),
    (*b"vvis", HeifBrandKind::ImageSequence),
];

pub(crate) fn heif_brand_kind(brand: &[u8]) -> HeifBrandKind {
    let Ok(brand) = <[u8; 4]>::try_from(brand) else {
        return HeifBrandKind::Unknown;
    };

    HEIF_BRAND_KINDS
        .iter()
        .find_map(|(candidate, kind)| (*candidate == brand).then_some(*kind))
        .unwrap_or(HeifBrandKind::Unknown)
}

pub(crate) fn is_heif_family_brand(brand: &[u8]) -> bool {
    heif_brand_kind(brand) != HeifBrandKind::Unknown
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn classifies_known_heif_brands_by_kind() {
        for (brand, kind) in HEIF_BRAND_KINDS {
            assert_eq!(heif_brand_kind(brand), *kind);
            assert!(is_heif_family_brand(brand));
        }
    }

    #[test]
    fn rejects_unknown_or_malformed_brands() {
        assert_eq!(heif_brand_kind(b"png "), HeifBrandKind::Unknown);
        assert_eq!(heif_brand_kind(b"too long"), HeifBrandKind::Unknown);
        assert!(!is_heif_family_brand(b"png "));
        assert!(!is_heif_family_brand(b"too long"));
    }
}
