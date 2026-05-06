// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView normalizes selected alpha AVIF/AVIFS BMFF
// metadata in memory before QImageReader sees the bytes because current
// KDE imageformats cannot read affected files without these fixes. Original
// files are not modified. Remove this module once KDE imageformats accepts
// those files without normalization.

use crate::{
    bmff::{
        BOX_HEADER_SIZE, BOX_KIND_OFFSET, BOX_KIND_SIZE, BoxHeader,
        FULL_BOX_VERSION_AND_FLAGS_SIZE, child_boxes, read_box, read_full_box, top_level_boxes,
    },
    byteio::{read_be_u16, read_be_u32, write_be_u16, write_be_u32},
};
use cxx_qt_lib::QByteArray;

const IPMA_ENTRY_COUNT_SIZE: usize = 4;
const IPMA_VERSION_AND_FLAGS_OFFSET: usize = BOX_HEADER_SIZE;
const IPMA_ENTRY_COUNT_OFFSET: usize =
    IPMA_VERSION_AND_FLAGS_OFFSET + FULL_BOX_VERSION_AND_FLAGS_SIZE;
const IPMA_ENTRIES_OFFSET: usize = IPMA_ENTRY_COUNT_OFFSET + IPMA_ENTRY_COUNT_SIZE;
const EMPTY_IPMA_BOX_SIZE: usize = IPMA_ENTRIES_OFFSET;

#[cxx::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qbytearray.h");

        type QByteArray = cxx_qt_lib::QByteArray;
    }

    #[namespace = "KiriView"]
    extern "Rust" {
        #[cxx_name = "avifDataWithCompatibilityFixes"]
        fn avif_data_with_compatibility_fixes(data: &QByteArray) -> QByteArray;
    }
}

struct IpmaBox {
    box_header: BoxHeader,
    version_and_flags: [u8; FULL_BOX_VERSION_AND_FLAGS_SIZE],
    entry_count: u32,
    entries: Vec<u8>,
}

struct MetaBox {
    child_boxes: Vec<BoxHeader>,
}

fn avif_data_with_compatibility_fixes(data: &QByteArray) -> QByteArray {
    fixed_avif_data(data.as_slice())
        .map(|fixed_data| QByteArray::from(fixed_data.as_slice()))
        .unwrap_or_else(|| data.clone())
}

fn has_avif_brand(data: &[u8], ftyp_box: BoxHeader) -> bool {
    if !ftyp_box.is_type(b"ftyp") || ftyp_box.size < ftyp_box.header_size + 8 {
        return false;
    }

    let mut offset = ftyp_box.body_offset();
    while offset + 4 <= ftyp_box.end_offset() {
        if let Some(brand) = data.get(offset..offset + 4)
            && (brand == b"avif" || brand == b"avis")
        {
            return true;
        }
        offset += 4;
    }

    false
}

fn boxes_contain_avif_brand(data: &[u8], boxes: &[BoxHeader]) -> bool {
    boxes
        .iter()
        .copied()
        .any(|box_header| has_avif_brand(data, box_header))
}

fn read_meta_box(data: &[u8], meta_box: BoxHeader) -> Option<MetaBox> {
    let full_box = read_full_box(data, meta_box, b"meta", 0)?;

    Some(MetaBox {
        child_boxes: child_boxes(data, full_box.payload_offset(), full_box.end_offset())?,
    })
}

fn primary_item_id(data: &[u8], meta_box: &MetaBox) -> Option<u32> {
    for box_header in &meta_box.child_boxes {
        let Some(full_box) = read_full_box(data, *box_header, b"pitm", 0) else {
            continue;
        };

        let item_id_size = primary_item_id_size(full_box.version())?;
        let item_id_offset = full_box.payload_offset();

        if item_id_offset
            .checked_add(item_id_size)
            .is_some_and(|end| end <= full_box.end_offset())
        {
            return read_item_id(data, item_id_offset, item_id_size);
        }
    }

    None
}

fn aux_c_contains_alpha(data: &[u8], aux_c_box: BoxHeader) -> bool {
    data.get(aux_c_box.body_offset()..aux_c_box.end_offset())
        .is_some_and(|body| {
            body.windows(b"alpha".len())
                .any(|window| window == b"alpha")
        })
}

fn iprp_child_box_groups(data: &[u8], meta_box: &MetaBox) -> Vec<Vec<BoxHeader>> {
    meta_box
        .child_boxes
        .iter()
        .filter(|box_header| box_header.is_type(b"iprp"))
        .filter_map(|box_header| {
            child_boxes(data, box_header.body_offset(), box_header.end_offset())
        })
        .collect()
}

fn has_alpha_auxiliary_property(data: &[u8], meta_box: &MetaBox) -> bool {
    for iprp_children in iprp_child_box_groups(data, meta_box) {
        for iprp_child in iprp_children {
            if !iprp_child.is_type(b"ipco") {
                continue;
            }

            let Some(property_boxes) =
                child_boxes(data, iprp_child.body_offset(), iprp_child.end_offset())
            else {
                continue;
            };
            for property_box in property_boxes {
                if property_box.is_type(b"auxC") && aux_c_contains_alpha(data, property_box) {
                    return true;
                }
            }
        }
    }

    false
}

fn primary_item_id_size(version: u8) -> Option<usize> {
    match version {
        0 => Some(2),
        1 => Some(4),
        _ => None,
    }
}

fn item_reference_id_size(version: u8) -> usize {
    if version == 1 { 4 } else { 2 }
}

fn read_item_id(data: &[u8], offset: usize, id_size: usize) -> Option<u32> {
    match id_size {
        2 => Some(u32::from(read_be_u16(data, offset)?)),
        4 => read_be_u32(data, offset),
        _ => None,
    }
}

fn write_item_id(data: &mut [u8], offset: usize, id_size: usize, item_id: u32) -> bool {
    match id_size {
        2 => u16::try_from(item_id)
            .ok()
            .is_some_and(|item_id| write_be_u16(data, offset, item_id)),
        4 => write_be_u32(data, offset, item_id),
        _ => false,
    }
}

fn patch_item_reference(data: &mut [u8], iref_box: BoxHeader, item_id: u32) -> bool {
    let Some(full_box) = read_full_box(data, iref_box, b"iref", 0) else {
        return false;
    };

    let mut changed = false;
    let id_size = item_reference_id_size(full_box.version());
    let mut offset = full_box.payload_offset();

    while offset < iref_box.end_offset() {
        let Some(reference_box) = read_box(data, offset, iref_box.end_offset()) else {
            return changed;
        };

        let mut cursor = reference_box.body_offset();
        if reference_box.end_offset() - cursor < id_size + 2 {
            return changed;
        }

        cursor += id_size;
        let Some(reference_count) = read_be_u16(data, cursor) else {
            return changed;
        };
        cursor += 2;

        if reference_box.is_type(b"auxl") {
            for _ in 0..reference_count {
                if cursor + id_size > reference_box.end_offset() {
                    return changed;
                }

                let Some(referenced_item_id) = read_item_id(data, cursor, id_size) else {
                    return changed;
                };
                if referenced_item_id == 0 {
                    if !write_item_id(data, cursor, id_size, item_id) {
                        return changed;
                    }
                    changed = true;
                }

                cursor += id_size;
            }
        }

        offset = reference_box.end_offset();
    }

    changed
}

fn patch_zero_auxl_references(data: &mut [u8], meta_box: &MetaBox) -> bool {
    let Some(item_id) = primary_item_id(data, meta_box) else {
        return false;
    };
    if item_id == 0 {
        return false;
    }

    let mut changed = false;
    for box_header in &meta_box.child_boxes {
        if box_header.is_type(b"iref") {
            changed = patch_item_reference(data, *box_header, item_id) || changed;
        }
    }

    changed
}

fn read_ipma_box(data: &[u8], box_header: BoxHeader) -> Option<IpmaBox> {
    let full_box = read_full_box(data, box_header, b"ipma", IPMA_ENTRY_COUNT_SIZE)?;
    if full_box.box_header.header_size != BOX_HEADER_SIZE {
        // The in-place IPMA merger emits standard 32-bit box headers.
        return None;
    }

    let entry_count = read_be_u32(data, full_box.payload_offset())?;
    let entries_offset = full_box
        .payload_offset()
        .checked_add(IPMA_ENTRY_COUNT_SIZE)?;
    let entries = data.get(entries_offset..full_box.end_offset())?.to_vec();

    Some(IpmaBox {
        box_header,
        version_and_flags: full_box.version_and_flags,
        entry_count,
        entries,
    })
}

fn ipma_box_size(entries_len: usize) -> Option<usize> {
    IPMA_ENTRIES_OFFSET.checked_add(entries_len)
}

fn write_box_header(data: &mut [u8], kind: &[u8; BOX_KIND_SIZE], size: usize) -> bool {
    if size > u32::MAX as usize || data.len() < BOX_HEADER_SIZE {
        return false;
    }

    if !write_be_u32(data, 0, size as u32) {
        return false;
    }
    data[BOX_KIND_OFFSET..BOX_KIND_OFFSET + BOX_KIND_SIZE].copy_from_slice(kind);
    true
}

fn write_ipma_box(
    data: &mut [u8],
    version_and_flags: [u8; FULL_BOX_VERSION_AND_FLAGS_SIZE],
    entry_count: u32,
    entries: &[u8],
) -> bool {
    let Some(size) = ipma_box_size(entries.len()) else {
        return false;
    };
    if data.len() != size || !write_box_header(data, b"ipma", size) {
        return false;
    }

    data[IPMA_VERSION_AND_FLAGS_OFFSET..IPMA_ENTRY_COUNT_OFFSET]
        .copy_from_slice(&version_and_flags);
    if !write_be_u32(data, IPMA_ENTRY_COUNT_OFFSET, entry_count) {
        return false;
    }
    data[IPMA_ENTRIES_OFFSET..].copy_from_slice(entries);
    true
}

fn empty_ipma_box_with_alternate_version(
    version_and_flags: [u8; FULL_BOX_VERSION_AND_FLAGS_SIZE],
) -> Vec<u8> {
    let mut box_data = vec![0; EMPTY_IPMA_BOX_SIZE];
    let mut alternate_version_and_flags = version_and_flags;
    alternate_version_and_flags[0] = if version_and_flags[0] == 0 { 1 } else { 0 };
    let wrote = write_ipma_box(&mut box_data, alternate_version_and_flags, 0, &[]);
    debug_assert!(wrote);
    box_data
}

fn patch_adjacent_duplicate_ipma_boxes(data: &mut [u8], first: &IpmaBox, second: &IpmaBox) -> bool {
    if first.version_and_flags != second.version_and_flags
        || first.box_header.end_offset() != second.box_header.offset
    {
        return false;
    }

    let mut entries = first.entries.clone();
    entries.extend_from_slice(&second.entries);

    let Some(merged_size) = ipma_box_size(entries.len()) else {
        return false;
    };
    let Some(available_size) = first.box_header.size.checked_add(second.box_header.size) else {
        return false;
    };
    let Some(remaining_size) = available_size.checked_sub(merged_size) else {
        return false;
    };

    let Some(entry_count) = first.entry_count.checked_add(second.entry_count) else {
        return false;
    };

    if remaining_size != EMPTY_IPMA_BOX_SIZE {
        return false;
    }

    let mut replacement = vec![0; merged_size];
    if !write_ipma_box(
        &mut replacement,
        first.version_and_flags,
        entry_count,
        &entries,
    ) {
        return false;
    }
    replacement.extend_from_slice(&empty_ipma_box_with_alternate_version(
        first.version_and_flags,
    ));

    if replacement.len() != available_size {
        return false;
    }

    let replacement_end = first.box_header.offset + available_size;
    let Some(target) = data.get_mut(first.box_header.offset..replacement_end) else {
        return false;
    };
    target.copy_from_slice(&replacement);
    true
}

fn patch_duplicate_ipma_boxes(data: &mut [u8], meta_box: &MetaBox) -> bool {
    let mut changed = false;

    for iprp_children in iprp_child_box_groups(data, meta_box) {
        let mut previous_ipma = None;
        for iprp_child in iprp_children {
            let Some(ipma) = read_ipma_box(data, iprp_child) else {
                previous_ipma = None;
                continue;
            };

            if let Some(previous) = &previous_ipma
                && patch_adjacent_duplicate_ipma_boxes(data, previous, &ipma)
            {
                changed = true;
                previous_ipma = None;
                continue;
            }

            previous_ipma = Some(ipma);
        }
    }

    changed
}

fn fixed_avif_data(data: &[u8]) -> Option<Vec<u8>> {
    let top_level_boxes = top_level_boxes(data)?;
    if !boxes_contain_avif_brand(data, &top_level_boxes) {
        return None;
    }

    let mut fixed_data = None;
    let mut changed = false;

    for box_header in top_level_boxes {
        if !box_header.is_type(b"meta") {
            continue;
        }
        let Some(meta_box) = read_meta_box(data, box_header) else {
            continue;
        };
        if !has_alpha_auxiliary_property(data, &meta_box) {
            continue;
        }

        let fixed_data = fixed_data.get_or_insert_with(|| data.to_vec());
        changed = patch_zero_auxl_references(fixed_data, &meta_box) || changed;
        changed = patch_duplicate_ipma_boxes(fixed_data, &meta_box) || changed;
    }

    changed.then_some(fixed_data?)
}

#[cfg(test)]
mod tests {
    use crate::bmff::test_support::{make_box, make_full_box, make_large_full_box};

    use super::*;

    fn make_ftyp(brand: &[u8; 4]) -> Vec<u8> {
        let mut body = Vec::new();
        body.extend_from_slice(brand);
        body.extend_from_slice(&0_u32.to_be_bytes());
        body.extend_from_slice(brand);
        make_box(b"ftyp", &body)
    }

    fn make_meta(children: &[Vec<u8>]) -> Vec<u8> {
        let body_size =
            FULL_BOX_VERSION_AND_FLAGS_SIZE + children.iter().map(Vec::len).sum::<usize>();
        let mut body = Vec::with_capacity(body_size);
        body.extend_from_slice(&[0, 0, 0, 0]);
        for child in children {
            body.extend_from_slice(child);
        }
        make_box(b"meta", &body)
    }

    fn make_alpha_iprp(mut children: Vec<Vec<u8>>) -> Vec<u8> {
        let aux_c = make_box(b"auxC", b"urn:mpeg:mpegB:cicp:systems:auxiliary:alpha");
        let ipco = make_box(b"ipco", &aux_c);
        let mut body = ipco;
        for child in children.drain(..) {
            body.extend_from_slice(&child);
        }
        make_box(b"iprp", &body)
    }

    fn make_avif_with_meta_children(children: &[Vec<u8>]) -> Vec<u8> {
        let mut data = make_ftyp(b"avif");
        data.extend_from_slice(&make_meta(children));
        data
    }

    fn find_bytes(haystack: &[u8], needle: &[u8]) -> Option<usize> {
        haystack
            .windows(needle.len())
            .position(|window| window == needle)
    }

    #[test]
    fn ignores_non_avif_data() {
        let data = make_ftyp(b"mif1");

        assert!(fixed_avif_data(&data).is_none());
    }

    #[test]
    fn patches_zero_auxl_reference_to_primary_item_id() {
        let pitm = make_full_box(b"pitm", [0, 0, 0, 0], &7_u16.to_be_bytes());
        let auxl = make_box(b"auxl", &[0, 2, 0, 1, 0, 0]);
        let iref = make_full_box(b"iref", [0, 0, 0, 0], &auxl);
        let iprp = make_alpha_iprp(Vec::new());
        let data = make_avif_with_meta_children(&[pitm, iprp, iref]);

        let fixed = fixed_avif_data(&data).expect("zero auxl reference should be patched");

        assert!(find_bytes(&data, &[0, 2, 0, 1, 0, 0]).is_some());
        assert!(find_bytes(&fixed, &[0, 2, 0, 1, 0, 7]).is_some());
    }

    #[test]
    fn patches_version_one_zero_auxl_reference_to_primary_item_id() {
        let pitm = make_full_box(b"pitm", [1, 0, 0, 0], &7_u32.to_be_bytes());
        let auxl = make_box(b"auxl", &[0, 0, 0, 2, 0, 1, 0, 0, 0, 0]);
        let iref = make_full_box(b"iref", [1, 0, 0, 0], &auxl);
        let iprp = make_alpha_iprp(Vec::new());
        let data = make_avif_with_meta_children(&[pitm, iprp, iref]);

        let fixed = fixed_avif_data(&data).expect("version-1 auxl reference should be patched");

        assert!(find_bytes(&data, &[0, 0, 0, 2, 0, 1, 0, 0, 0, 0]).is_some());
        assert!(find_bytes(&fixed, &[0, 0, 0, 2, 0, 1, 0, 0, 0, 7]).is_some());
    }

    #[test]
    fn merges_adjacent_duplicate_ipma_boxes_and_keeps_size_stable() {
        let first_ipma = make_full_box(b"ipma", [0, 0, 0, 0], &[0, 0, 0, 1, 1, 2, 3]);
        let second_ipma = make_full_box(b"ipma", [0, 0, 0, 0], &[0, 0, 0, 1, 4, 5]);
        let iprp = make_alpha_iprp(vec![first_ipma, second_ipma]);
        let data = make_avif_with_meta_children(&[iprp]);

        let fixed = fixed_avif_data(&data).expect("duplicate ipma boxes should be patched");

        let expected = [
            0, 0, 0, 21, b'i', b'p', b'm', b'a', 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 3, 4, 5, 0, 0, 0,
            16, b'i', b'p', b'm', b'a', 1, 0, 0, 0, 0, 0, 0, 0,
        ];
        assert_eq!(fixed.len(), data.len());
        assert!(find_bytes(&fixed, &expected).is_some());
    }

    #[test]
    fn leaves_large_duplicate_ipma_boxes_unmodified() {
        let first_ipma = make_large_full_box(b"ipma", [0, 0, 0, 0], &[0, 0, 0, 1, 1, 2, 3]);
        let second_ipma = make_large_full_box(b"ipma", [0, 0, 0, 0], &[0, 0, 0, 1, 4, 5]);
        let iprp = make_alpha_iprp(vec![first_ipma, second_ipma]);
        let data = make_avif_with_meta_children(&[iprp]);

        assert!(fixed_avif_data(&data).is_none());
    }
}
