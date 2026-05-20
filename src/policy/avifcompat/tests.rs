// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

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
    let body_size = FULL_BOX_VERSION_AND_FLAGS_SIZE + children.iter().map(Vec::len).sum::<usize>();
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
        0, 0, 0, 21, b'i', b'p', b'm', b'a', 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 3, 4, 5, 0, 0, 0, 16,
        b'i', b'p', b'm', b'a', 1, 0, 0, 0, 0, 0, 0, 0,
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
