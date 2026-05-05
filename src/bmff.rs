// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::byteio::{read_be_u32, read_be_u64};

pub(crate) const BOX_HEADER_SIZE: usize = 8;
pub(crate) const BOX_KIND_SIZE: usize = 4;
pub(crate) const BOX_KIND_OFFSET: usize = 4;
const LARGE_BOX_EXTRA_SIZE: usize = 8;

pub(crate) type BoxKind = [u8; BOX_KIND_SIZE];

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct BoxHeader {
    pub(crate) offset: usize,
    pub(crate) size: usize,
    pub(crate) header_size: usize,
    pub(crate) kind: BoxKind,
}

impl BoxHeader {
    pub(crate) fn body_offset(self) -> usize {
        self.offset + self.header_size
    }

    pub(crate) fn end_offset(self) -> usize {
        self.offset + self.size
    }

    pub(crate) fn is_type(self, expected: &BoxKind) -> bool {
        self.kind == *expected
    }
}

pub(crate) fn read_box(data: &[u8], offset: usize, end_offset: usize) -> Option<BoxHeader> {
    if end_offset > data.len()
        || end_offset < offset
        || end_offset.checked_sub(offset)? < BOX_HEADER_SIZE
    {
        return None;
    }

    let small_size = read_be_u32(data, offset)?;
    let kind = data
        .get(offset + BOX_KIND_OFFSET..offset + BOX_KIND_OFFSET + BOX_KIND_SIZE)?
        .try_into()
        .ok()?;
    let mut header_size = BOX_HEADER_SIZE;
    let mut size = u64::from(small_size);

    if small_size == 1 {
        if end_offset - offset < BOX_HEADER_SIZE + LARGE_BOX_EXTRA_SIZE {
            return None;
        }
        size = read_be_u64(data, offset + BOX_HEADER_SIZE)?;
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

pub(crate) fn child_boxes(data: &[u8], mut offset: usize, end_offset: usize) -> Vec<BoxHeader> {
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

pub(crate) fn top_level_boxes(data: &[u8]) -> Vec<BoxHeader> {
    child_boxes(data, 0, data.len())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_box(kind: &BoxKind, body: &[u8]) -> Vec<u8> {
        let size = BOX_HEADER_SIZE + body.len();
        let mut data = Vec::with_capacity(size);
        data.extend_from_slice(&(size as u32).to_be_bytes());
        data.extend_from_slice(kind);
        data.extend_from_slice(body);
        data
    }

    fn make_large_box(kind: &BoxKind, body: &[u8]) -> Vec<u8> {
        let size = BOX_HEADER_SIZE + LARGE_BOX_EXTRA_SIZE + body.len();
        let mut data = Vec::with_capacity(size);
        data.extend_from_slice(&1_u32.to_be_bytes());
        data.extend_from_slice(kind);
        data.extend_from_slice(&(size as u64).to_be_bytes());
        data.extend_from_slice(body);
        data
    }

    #[test]
    fn reads_standard_box_header() {
        let data = make_box(b"ftyp", &[1, 2, 3, 4]);

        let header = read_box(&data, 0, data.len()).expect("box should parse");

        assert_eq!(
            header,
            BoxHeader {
                offset: 0,
                size: 12,
                header_size: BOX_HEADER_SIZE,
                kind: *b"ftyp",
            }
        );
        assert_eq!(header.body_offset(), 8);
        assert_eq!(header.end_offset(), 12);
        assert!(header.is_type(b"ftyp"));
    }

    #[test]
    fn reads_large_box_header() {
        let data = make_large_box(b"meta", &[1, 2, 3, 4]);

        let header = read_box(&data, 0, data.len()).expect("large box should parse");

        assert_eq!(header.size, 20);
        assert_eq!(header.header_size, BOX_HEADER_SIZE + LARGE_BOX_EXTRA_SIZE);
        assert_eq!(header.body_offset(), 16);
        assert!(header.is_type(b"meta"));
    }

    #[test]
    fn reads_size_zero_box_to_scan_end() {
        let mut data = Vec::new();
        data.extend_from_slice(&0_u32.to_be_bytes());
        data.extend_from_slice(b"free");
        data.extend_from_slice(&[1, 2, 3, 4]);

        let header = read_box(&data, 0, data.len()).expect("box should extend to scan end");

        assert_eq!(header.size, data.len());
        assert_eq!(header.end_offset(), data.len());
    }

    #[test]
    fn child_boxes_rejects_invalid_sequences() {
        let mut data = make_box(b"free", &[]);
        data.extend_from_slice(&[0, 0, 0]);

        assert!(read_box(&data[..3], 0, 3).is_none());
        assert!(child_boxes(&data, 0, data.len()).is_empty());
    }

    #[test]
    fn rejects_impossible_box_sizes() {
        let too_small = [0, 0, 0, 4, b'f', b'r', b'e', b'e'];
        let too_large = [0, 0, 0, 12, b'f', b'r', b'e', b'e'];

        assert!(read_box(&too_small, 0, too_small.len()).is_none());
        assert!(read_box(&too_large, 0, too_large.len()).is_none());
        assert!(read_box(&[], usize::MAX, 0).is_none());
    }
}
