// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView normalizes selected alpha AVIF/AVIFS BMFF
// metadata in memory before QImageReader sees the bytes because current
// KDE imageformats cannot read affected files without these fixes. Original
// files are not modified. Remove this module once KDE imageformats accepts
// those files without normalization.

use cxx_qt_lib::QByteArray;

const BOX_HEADER_SIZE: usize = 8;
const LARGE_BOX_EXTRA_SIZE: usize = 8;
const FULL_BOX_HEADER_SIZE: usize = 4;
const IPMA_ENTRY_COUNT_SIZE: usize = 4;
const EMPTY_IPMA_BOX_SIZE: usize = 16;

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

#[derive(Clone, Copy)]
struct BoxHeader {
    offset: usize,
    size: usize,
    header_size: usize,
    kind: [u8; 4],
}

impl BoxHeader {
    fn body_offset(self) -> usize {
        self.offset + self.header_size
    }

    fn end_offset(self) -> usize {
        self.offset + self.size
    }
}

struct IpmaBox {
    box_header: BoxHeader,
    version_and_flags: [u8; FULL_BOX_HEADER_SIZE],
    entry_count: u32,
    entries: Vec<u8>,
}

fn avif_data_with_compatibility_fixes(data: &QByteArray) -> QByteArray {
    fixed_avif_data(data.as_slice())
        .map(|fixed_data| QByteArray::from(fixed_data.as_slice()))
        .unwrap_or_else(|| data.clone())
}

fn read_u16(data: &[u8], offset: usize) -> Option<u16> {
    let bytes: [u8; 2] = data.get(offset..offset.checked_add(2)?)?.try_into().ok()?;
    Some(u16::from_be_bytes(bytes))
}

fn read_u32(data: &[u8], offset: usize) -> Option<u32> {
    let bytes: [u8; 4] = data.get(offset..offset.checked_add(4)?)?.try_into().ok()?;
    Some(u32::from_be_bytes(bytes))
}

fn read_u64(data: &[u8], offset: usize) -> Option<u64> {
    let bytes: [u8; 8] = data.get(offset..offset.checked_add(8)?)?.try_into().ok()?;
    Some(u64::from_be_bytes(bytes))
}

fn write_u16(data: &mut [u8], offset: usize, value: u16) -> bool {
    let Some(target) = data.get_mut(offset..offset.saturating_add(2)) else {
        return false;
    };
    target.copy_from_slice(&value.to_be_bytes());
    true
}

fn write_u32(data: &mut [u8], offset: usize, value: u32) -> bool {
    let Some(target) = data.get_mut(offset..offset.saturating_add(4)) else {
        return false;
    };
    target.copy_from_slice(&value.to_be_bytes());
    true
}

fn type_is(kind: [u8; 4], expected: &[u8; 4]) -> bool {
    kind == *expected
}

fn read_box(data: &[u8], offset: usize, end_offset: usize) -> Option<BoxHeader> {
    if end_offset > data.len()
        || end_offset < offset
        || end_offset.checked_sub(offset)? < BOX_HEADER_SIZE
    {
        return None;
    }

    let small_size = read_u32(data, offset)?;
    let kind = data.get(offset + 4..offset + 8)?.try_into().ok()?;
    let mut header_size = BOX_HEADER_SIZE;
    let mut size = u64::from(small_size);

    if small_size == 1 {
        if end_offset - offset < BOX_HEADER_SIZE + LARGE_BOX_EXTRA_SIZE {
            return None;
        }
        size = read_u64(data, offset + BOX_HEADER_SIZE)?;
        header_size += LARGE_BOX_EXTRA_SIZE;
    } else if small_size == 0 {
        size = u64::try_from(end_offset - offset).ok()?;
    }

    if size < u64::try_from(header_size).ok()? || size > usize::MAX as u64 {
        return None;
    }

    let size = usize::try_from(size).ok()?;
    if size > end_offset - offset {
        return None;
    }

    Some(BoxHeader {
        offset,
        size,
        header_size,
        kind,
    })
}

fn child_boxes(data: &[u8], mut offset: usize, end_offset: usize) -> Vec<BoxHeader> {
    let mut boxes = Vec::new();

    while offset < end_offset {
        let Some(box_header) = read_box(data, offset, end_offset) else {
            return Vec::new();
        };

        boxes.push(box_header);
        offset = box_header.end_offset();
    }

    boxes
}

fn top_level_boxes(data: &[u8]) -> Vec<BoxHeader> {
    child_boxes(data, 0, data.len())
}

fn has_avif_brand(data: &[u8], ftyp_box: BoxHeader) -> bool {
    if !type_is(ftyp_box.kind, b"ftyp") || ftyp_box.size < ftyp_box.header_size + 8 {
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

fn is_avif_file(data: &[u8]) -> bool {
    top_level_boxes(data)
        .into_iter()
        .any(|box_header| has_avif_brand(data, box_header))
}

fn meta_child_boxes(data: &[u8], meta_box: BoxHeader) -> Vec<BoxHeader> {
    if !type_is(meta_box.kind, b"meta")
        || meta_box.size < meta_box.header_size + FULL_BOX_HEADER_SIZE
    {
        return Vec::new();
    }

    child_boxes(
        data,
        meta_box.body_offset() + FULL_BOX_HEADER_SIZE,
        meta_box.end_offset(),
    )
}

fn primary_item_id(data: &[u8], meta_box: BoxHeader) -> Option<u32> {
    for box_header in meta_child_boxes(data, meta_box) {
        if !type_is(box_header.kind, b"pitm")
            || box_header.size < box_header.header_size + FULL_BOX_HEADER_SIZE + 2
        {
            continue;
        }

        let version_offset = box_header.body_offset();
        let version = *data.get(version_offset)?;
        let item_id_offset = version_offset + FULL_BOX_HEADER_SIZE;

        if version == 0 && item_id_offset + 2 <= box_header.end_offset() {
            return Some(u32::from(read_u16(data, item_id_offset)?));
        }
        if version == 1 && item_id_offset + 4 <= box_header.end_offset() {
            return read_u32(data, item_id_offset);
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

fn has_alpha_auxiliary_property(data: &[u8], meta_box: BoxHeader) -> bool {
    for box_header in meta_child_boxes(data, meta_box) {
        if !type_is(box_header.kind, b"iprp") {
            continue;
        }

        for iprp_child in child_boxes(data, box_header.body_offset(), box_header.end_offset()) {
            if !type_is(iprp_child.kind, b"ipco") {
                continue;
            }

            for property_box in child_boxes(data, iprp_child.body_offset(), iprp_child.end_offset())
            {
                if type_is(property_box.kind, b"auxC") && aux_c_contains_alpha(data, property_box) {
                    return true;
                }
            }
        }
    }

    false
}

fn patch_item_reference(data: &mut [u8], iref_box: BoxHeader, item_id: u32) -> bool {
    if iref_box.size < iref_box.header_size + FULL_BOX_HEADER_SIZE {
        return false;
    }

    let mut changed = false;
    let Some(version) = data.get(iref_box.body_offset()).copied() else {
        return false;
    };
    let id_size = if version == 1 { 4 } else { 2 };
    let mut offset = iref_box.body_offset() + FULL_BOX_HEADER_SIZE;

    while offset < iref_box.end_offset() {
        let Some(reference_box) = read_box(data, offset, iref_box.end_offset()) else {
            return changed;
        };

        let mut cursor = reference_box.body_offset();
        if reference_box.end_offset() - cursor < id_size + 2 {
            return changed;
        }

        cursor += id_size;
        let Some(reference_count) = read_u16(data, cursor) else {
            return changed;
        };
        cursor += 2;

        if type_is(reference_box.kind, b"auxl") {
            for _ in 0..reference_count {
                if cursor + id_size > reference_box.end_offset() {
                    return changed;
                }

                let referenced_item_id = if id_size == 4 {
                    read_u32(data, cursor).unwrap_or_default()
                } else {
                    u32::from(read_u16(data, cursor).unwrap_or_default())
                };
                if referenced_item_id == 0 {
                    if id_size == 4 {
                        if !write_u32(data, cursor, item_id) {
                            return changed;
                        }
                    } else if let Ok(item_id) = u16::try_from(item_id) {
                        if !write_u16(data, cursor, item_id) {
                            return changed;
                        }
                    } else {
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

fn patch_zero_auxl_references(data: &mut [u8], meta_box: BoxHeader) -> bool {
    let Some(item_id) = primary_item_id(data, meta_box) else {
        return false;
    };
    if item_id == 0 {
        return false;
    }

    let mut changed = false;
    for box_header in meta_child_boxes(data, meta_box) {
        if type_is(box_header.kind, b"iref") {
            changed = patch_item_reference(data, box_header, item_id) || changed;
        }
    }

    changed
}

fn read_ipma_box(data: &[u8], box_header: BoxHeader) -> Option<IpmaBox> {
    if !type_is(box_header.kind, b"ipma")
        || box_header.size < box_header.header_size + FULL_BOX_HEADER_SIZE + IPMA_ENTRY_COUNT_SIZE
    {
        return None;
    }

    let body_offset = box_header.body_offset();
    let version_and_flags = data
        .get(body_offset..body_offset + FULL_BOX_HEADER_SIZE)?
        .try_into()
        .ok()?;
    let entry_count = read_u32(data, body_offset + FULL_BOX_HEADER_SIZE)?;
    let entries = data
        .get(body_offset + FULL_BOX_HEADER_SIZE + IPMA_ENTRY_COUNT_SIZE..box_header.end_offset())?
        .to_vec();

    Some(IpmaBox {
        box_header,
        version_and_flags,
        entry_count,
        entries,
    })
}

fn empty_ipma_box_with_alternate_version(version_and_flags: [u8; FULL_BOX_HEADER_SIZE]) -> Vec<u8> {
    let mut box_data = vec![0; EMPTY_IPMA_BOX_SIZE];
    write_u32(&mut box_data, 0, EMPTY_IPMA_BOX_SIZE as u32);
    box_data[4..8].copy_from_slice(b"ipma");
    box_data[8] = if version_and_flags[0] == 0 { 1 } else { 0 };
    box_data[9..12].copy_from_slice(&version_and_flags[1..4]);
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

    let Some(merged_size) = BOX_HEADER_SIZE
        .checked_add(FULL_BOX_HEADER_SIZE)
        .and_then(|size| size.checked_add(IPMA_ENTRY_COUNT_SIZE))
        .and_then(|size| size.checked_add(entries.len()))
    else {
        return false;
    };
    let Some(available_size) = first.box_header.size.checked_add(second.box_header.size) else {
        return false;
    };
    let Some(remaining_size) = available_size.checked_sub(merged_size) else {
        return false;
    };

    if remaining_size != EMPTY_IPMA_BOX_SIZE
        || merged_size > u32::MAX as usize
        || first.entry_count > u32::MAX - second.entry_count
    {
        return false;
    }

    let mut replacement = vec![0; merged_size];
    write_u32(&mut replacement, 0, merged_size as u32);
    replacement[4..8].copy_from_slice(b"ipma");
    replacement[8..12].copy_from_slice(&first.version_and_flags);
    write_u32(&mut replacement, 12, first.entry_count + second.entry_count);
    replacement[16..].copy_from_slice(&entries);
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

fn patch_duplicate_ipma_boxes(data: &mut [u8], meta_box: BoxHeader) -> bool {
    let mut changed = false;

    for box_header in meta_child_boxes(data, meta_box) {
        if !type_is(box_header.kind, b"iprp") {
            continue;
        }

        let mut previous_ipma = None;
        for iprp_child in child_boxes(data, box_header.body_offset(), box_header.end_offset()) {
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
    if !is_avif_file(data) {
        return None;
    }

    let mut fixed_data = None;
    let mut changed = false;

    for box_header in top_level_boxes(data) {
        if !type_is(box_header.kind, b"meta") || !has_alpha_auxiliary_property(data, box_header) {
            continue;
        }

        let fixed_data = fixed_data.get_or_insert_with(|| data.to_vec());
        changed = patch_zero_auxl_references(fixed_data, box_header) || changed;
        changed = patch_duplicate_ipma_boxes(fixed_data, box_header) || changed;
    }

    changed.then_some(fixed_data?)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_box(kind: &[u8; 4], body: &[u8]) -> Vec<u8> {
        let size = BOX_HEADER_SIZE + body.len();
        let mut data = Vec::with_capacity(size);
        data.extend_from_slice(&(size as u32).to_be_bytes());
        data.extend_from_slice(kind);
        data.extend_from_slice(body);
        data
    }

    fn make_full_box(kind: &[u8; 4], version_and_flags: [u8; 4], body: &[u8]) -> Vec<u8> {
        let mut full_body = Vec::with_capacity(FULL_BOX_HEADER_SIZE + body.len());
        full_body.extend_from_slice(&version_and_flags);
        full_body.extend_from_slice(body);
        make_box(kind, &full_body)
    }

    fn make_ftyp(brand: &[u8; 4]) -> Vec<u8> {
        let mut body = Vec::new();
        body.extend_from_slice(brand);
        body.extend_from_slice(&0_u32.to_be_bytes());
        body.extend_from_slice(brand);
        make_box(b"ftyp", &body)
    }

    fn make_meta(children: &[Vec<u8>]) -> Vec<u8> {
        let body_size = FULL_BOX_HEADER_SIZE + children.iter().map(Vec::len).sum::<usize>();
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
}
