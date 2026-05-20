// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pub(crate) fn read_be_u16(data: &[u8], offset: usize) -> Option<u16> {
    Some(u16::from_be_bytes(read_be_array(data, offset)?))
}

pub(crate) fn read_be_u32(data: &[u8], offset: usize) -> Option<u32> {
    Some(u32::from_be_bytes(read_be_array(data, offset)?))
}

pub(crate) fn read_be_u64(data: &[u8], offset: usize) -> Option<u64> {
    Some(u64::from_be_bytes(read_be_array(data, offset)?))
}

pub(crate) fn write_be_u16(data: &mut [u8], offset: usize, value: u16) -> bool {
    write_be_bytes(data, offset, &value.to_be_bytes())
}

pub(crate) fn write_be_u32(data: &mut [u8], offset: usize, value: u32) -> bool {
    write_be_bytes(data, offset, &value.to_be_bytes())
}

fn read_be_array<const N: usize>(data: &[u8], offset: usize) -> Option<[u8; N]> {
    data.get(offset..offset.checked_add(N)?)?.try_into().ok()
}

fn write_be_bytes(data: &mut [u8], offset: usize, bytes: &[u8]) -> bool {
    let Some(end) = offset.checked_add(bytes.len()) else {
        return false;
    };
    let Some(target) = data.get_mut(offset..end) else {
        return false;
    };

    target.copy_from_slice(bytes);
    true
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reads_big_endian_values_with_bounds_checks() {
        let data = [0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0];

        assert_eq!(read_be_u16(&data, 1), Some(0x3456));
        assert_eq!(read_be_u32(&data, 2), Some(0x5678_9abc));
        assert_eq!(read_be_u64(&data, 0), Some(0x1234_5678_9abc_def0));
        assert_eq!(read_be_u32(&data, 5), None);
        assert_eq!(read_be_u16(&data, usize::MAX), None);
    }

    #[test]
    fn writes_big_endian_values_with_bounds_checks() {
        let mut data = [0; 6];

        assert!(write_be_u16(&mut data, 1, 0x1234));
        assert!(write_be_u32(&mut data, 2, 0x5678_9abc));
        assert_eq!(data, [0, 0x12, 0x56, 0x78, 0x9a, 0xbc]);
        assert!(!write_be_u32(&mut data, 3, 0));
        assert!(!write_be_u16(&mut data, usize::MAX, 0));
    }
}
