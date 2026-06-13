// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use std::{collections::BTreeSet, path::Path};

use nom_exif::{
    EntryValue, Exif, ExifTag, GPSInfo, IfdIndex, MediaParser, MediaSource, TrackInfo,
    TrackInfoTag, URational,
};

#[cxx::bridge(namespace = "kiriview")]
mod ffi {
    #[derive(Clone, Debug, PartialEq, Eq)]
    struct RustEmbeddedMetadataRow {
        label: String,
        value: String,
    }

    #[derive(Clone, Debug, Default, PartialEq, Eq)]
    struct RustEmbeddedMetadata {
        camera_make: String,
        camera_model: String,
        taken: String,
        location: String,
        lens: String,
        exposure: String,
        iso: String,
        focal_length: String,
        software: String,
        duration: String,
        frame_size: String,
        advanced_rows: Vec<RustEmbeddedMetadataRow>,
    }

    extern "Rust" {
        #[cxx_name = "rustParseImageEmbeddedMetadata"]
        fn rust_parse_image_embedded_metadata(data: &[u8]) -> RustEmbeddedMetadata;

        #[cxx_name = "rustParsePathEmbeddedMetadata"]
        fn rust_parse_path_embedded_metadata(path: &str) -> RustEmbeddedMetadata;
    }
}

pub(super) use ffi::{RustEmbeddedMetadata, RustEmbeddedMetadataRow};

fn rust_parse_image_embedded_metadata(data: &[u8]) -> RustEmbeddedMetadata {
    parse_image_metadata(data)
}

fn rust_parse_path_embedded_metadata(path: &str) -> RustEmbeddedMetadata {
    parse_path_metadata(path)
}

fn parse_image_metadata(data: &[u8]) -> RustEmbeddedMetadata {
    let Ok(source) = MediaSource::from_memory(data.to_vec()) else {
        return RustEmbeddedMetadata::default();
    };
    let mut parser = MediaParser::new();
    let Ok(iter) = parser.parse_exif(source) else {
        return RustEmbeddedMetadata::default();
    };

    let gps = iter.parse_gps().ok().flatten();
    let exif: Exif = iter.into();
    metadata_from_exif(&exif, gps.as_ref())
}

fn parse_path_metadata(path: &str) -> RustEmbeddedMetadata {
    let Ok(source) = MediaSource::open(Path::new(path)) else {
        return RustEmbeddedMetadata::default();
    };
    let mut parser = MediaParser::new();
    let Ok(track) = parser.parse_track(source) else {
        return RustEmbeddedMetadata::default();
    };

    metadata_from_track(&track)
}

fn metadata_from_exif(exif: &Exif, gps: Option<&GPSInfo>) -> RustEmbeddedMetadata {
    let mut metadata = RustEmbeddedMetadata {
        camera_make: text(exif.get(ExifTag::Make)),
        camera_model: text(exif.get(ExifTag::Model)),
        taken: datetime_text(exif.get(ExifTag::DateTimeOriginal))
            .or_else(|| datetime_text(exif.get(ExifTag::CreateDate)))
            .unwrap_or_default(),
        location: location_text(gps),
        lens: text(exif.get(ExifTag::LensModel)),
        exposure: exposure_text(exif),
        iso: number_text(exif.get(ExifTag::ISOSpeedRatings)),
        focal_length: focal_length_text(exif.get(ExifTag::FocalLength)),
        software: text(exif.get(ExifTag::Software)),
        duration: String::new(),
        frame_size: String::new(),
        advanced_rows: Vec::new(),
    };
    metadata.advanced_rows = exif_advanced_rows(exif, exif_consumed_tags());
    metadata
}

fn metadata_from_track(track: &TrackInfo) -> RustEmbeddedMetadata {
    let width = track.get(TrackInfoTag::Width).and_then(EntryValue::as_u32);
    let height = track.get(TrackInfoTag::Height).and_then(EntryValue::as_u32);
    let mut consumed = BTreeSet::new();
    let duration = track
        .get(TrackInfoTag::DurationMs)
        .and_then(EntryValue::as_u64)
        .map(|duration| {
            consumed.insert(TrackInfoTag::DurationMs);
            duration_text(duration)
        })
        .unwrap_or_default();
    let frame_size = width
        .zip(height)
        .map(|(width, height)| {
            consumed.insert(TrackInfoTag::Width);
            consumed.insert(TrackInfoTag::Height);
            format!("{width}×{height} px")
        })
        .unwrap_or_default();

    RustEmbeddedMetadata {
        camera_make: track_text(track, TrackInfoTag::Make),
        camera_model: track_text(track, TrackInfoTag::Model),
        taken: track
            .get(TrackInfoTag::CreateDate)
            .and_then(|value| datetime_text(Some(value)))
            .unwrap_or_default(),
        location: location_text(track.gps_info()),
        lens: String::new(),
        exposure: String::new(),
        iso: String::new(),
        focal_length: String::new(),
        software: track_text(track, TrackInfoTag::Software),
        duration,
        frame_size,
        advanced_rows: track_advanced_rows(track, &consumed),
    }
}

fn text(value: Option<&EntryValue>) -> String {
    value
        .and_then(EntryValue::as_str)
        .map(str::trim)
        .filter(|value| !value.is_empty())
        .unwrap_or_default()
        .to_owned()
}

fn track_text(track: &TrackInfo, tag: TrackInfoTag) -> String {
    text(track.get(tag))
}

fn number_text(value: Option<&EntryValue>) -> String {
    value
        .map(ToString::to_string)
        .map(|value| value.trim().to_owned())
        .filter(|value| !value.is_empty())
        .unwrap_or_default()
}

fn datetime_text(value: Option<&EntryValue>) -> Option<String> {
    value.and_then(EntryValue::as_datetime).map(|datetime| {
        datetime
            .into_naive()
            .format("%Y-%m-%d %H:%M:%S")
            .to_string()
    })
}

fn ratio_value(value: URational) -> Option<f64> {
    value.to_f64().filter(|value| value.is_finite())
}

fn exposure_time_text(value: URational) -> Option<String> {
    if value.denominator() == 0 {
        return None;
    }
    if value.numerator() == 1 {
        return Some(format!("1/{} s", value.denominator()));
    }
    ratio_value(value).map(|seconds| format!("{seconds:.4} s"))
}

fn aperture_text(value: URational) -> Option<String> {
    ratio_value(value).map(|aperture| format!("f/{aperture:.1}"))
}

fn exposure_text(exif: &Exif) -> String {
    let exposure = exif
        .get(ExifTag::ExposureTime)
        .and_then(EntryValue::as_urational)
        .and_then(exposure_time_text);
    let aperture = exif
        .get(ExifTag::FNumber)
        .and_then(EntryValue::as_urational)
        .and_then(aperture_text);

    match (exposure, aperture) {
        (Some(exposure), Some(aperture)) => format!("{exposure} at {aperture}"),
        (Some(exposure), None) => exposure,
        (None, Some(aperture)) => aperture,
        (None, None) => String::new(),
    }
}

fn focal_length_text(value: Option<&EntryValue>) -> String {
    value
        .and_then(EntryValue::as_urational)
        .and_then(ratio_value)
        .map(|length| {
            if (length.round() - length).abs() < 0.05 {
                format!("{} mm", length.round() as i64)
            } else {
                format!("{length:.1} mm")
            }
        })
        .unwrap_or_default()
}

fn location_text(gps: Option<&GPSInfo>) -> String {
    let Some(gps) = gps else {
        return String::new();
    };
    let Some(latitude) = gps.latitude_decimal() else {
        return String::new();
    };
    let Some(longitude) = gps.longitude_decimal() else {
        return String::new();
    };
    format!("{latitude:.4}, {longitude:.4}")
}

fn duration_text(duration_ms: u64) -> String {
    let hours = duration_ms / 3_600_000;
    let minutes = (duration_ms / 60_000) % 60;
    let seconds = (duration_ms / 1_000) % 60;
    let milliseconds = duration_ms % 1_000;
    format!("{hours:02}:{minutes:02}:{seconds:02}.{milliseconds:03}")
}

fn printable_value(value: &EntryValue) -> Option<String> {
    match value {
        EntryValue::Undefined(_) | EntryValue::U8Array(_) => None,
        _ => {
            let text = value.to_string();
            let trimmed = text.trim();
            (!trimmed.is_empty()
                && !trimmed.contains('\0')
                && trimmed.chars().all(|character| !character.is_control()))
            .then(|| trimmed.to_owned())
        }
    }
}

fn exif_consumed_tags() -> BTreeSet<u16> {
    [
        ExifTag::Make,
        ExifTag::Model,
        ExifTag::DateTimeOriginal,
        ExifTag::CreateDate,
        ExifTag::GPSInfo,
        ExifTag::GPSLatitudeRef,
        ExifTag::GPSLatitude,
        ExifTag::GPSLongitudeRef,
        ExifTag::GPSLongitude,
        ExifTag::LensModel,
        ExifTag::ExposureTime,
        ExifTag::FNumber,
        ExifTag::ISOSpeedRatings,
        ExifTag::FocalLength,
        ExifTag::Software,
    ]
    .into_iter()
    .map(ExifTag::code)
    .collect()
}

fn exif_advanced_rows(exif: &Exif, consumed: BTreeSet<u16>) -> Vec<RustEmbeddedMetadataRow> {
    exif.iter()
        .filter(|entry| entry.ifd != IfdIndex::THUMBNAIL)
        .filter(|entry| !consumed.contains(&entry.tag.code()))
        .filter_map(|entry| {
            Some(RustEmbeddedMetadataRow {
                label: entry.tag.to_string(),
                value: printable_value(entry.value)?,
            })
        })
        .collect()
}

fn track_advanced_rows(
    track: &TrackInfo,
    consumed: &BTreeSet<TrackInfoTag>,
) -> Vec<RustEmbeddedMetadataRow> {
    track
        .iter()
        .filter(|(tag, _)| !consumed.contains(tag))
        .filter_map(|(tag, value)| {
            Some(RustEmbeddedMetadataRow {
                label: tag.to_string(),
                value: printable_value(value)?,
            })
        })
        .collect()
}

#[cfg(test)]
impl RustEmbeddedMetadata {
    fn is_empty(&self) -> bool {
        self.camera_make.is_empty()
            && self.camera_model.is_empty()
            && self.taken.is_empty()
            && self.location.is_empty()
            && self.lens.is_empty()
            && self.exposure.is_empty()
            && self.iso.is_empty()
            && self.focal_length.is_empty()
            && self.software.is_empty()
            && self.duration.is_empty()
            && self.frame_size.is_empty()
            && self.advanced_rows.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{fs, time::SystemTime};

    const TIFF_HEADER_SIZE: u32 = 8;
    const IFD_ENTRY_SIZE: u32 = 12;

    #[derive(Clone)]
    struct IfdEntry {
        tag: u16,
        format: u16,
        count: u32,
        value: Vec<u8>,
    }

    struct TestExifJpegBuilder {
        ifd0: Vec<IfdEntry>,
        exif: Vec<IfdEntry>,
        gps: Vec<IfdEntry>,
    }

    impl TestExifJpegBuilder {
        fn new() -> Self {
            Self {
                ifd0: Vec::new(),
                exif: Vec::new(),
                gps: Vec::new(),
            }
        }

        fn ascii(mut self, tag: u16, value: &str) -> Self {
            let entry = ascii_entry(tag, value);
            if matches!(tag, 0x9003 | 0xa434 | 0x829a | 0x829d | 0x8827 | 0x920a) {
                self.exif.push(entry);
            } else {
                self.ifd0.push(entry);
            }
            self
        }

        fn rational(mut self, tag: u16, numerator: u32, denominator: u32) -> Self {
            self.exif.push(rational_entry(tag, numerator, denominator));
            self
        }

        fn short(mut self, tag: u16, value: u16) -> Self {
            self.exif.push(short_entry(tag, value));
            self
        }

        fn gps_ref(mut self, tag: u16, value: &str) -> Self {
            self.gps.push(ascii_entry(tag, value));
            self
        }

        fn gps_rational_triplet(mut self, tag: u16, values: [(u32, u32); 3]) -> Self {
            self.gps.push(rational_array_entry(tag, &values));
            self
        }

        fn finish(mut self) -> Vec<u8> {
            self.ifd0.push(long_entry(ExifTag::ExifOffset.code(), 0));
            self.ifd0.push(long_entry(ExifTag::GPSInfo.code(), 0));

            let ifd0_offset = TIFF_HEADER_SIZE;
            let ifd0_size = ifd_size(&self.ifd0);
            let exif_offset = ifd0_offset + ifd0_size;
            let exif_size = ifd_size(&self.exif);
            let gps_offset = exif_offset + exif_size;

            set_long_value(&mut self.ifd0, ExifTag::ExifOffset.code(), exif_offset);
            set_long_value(&mut self.ifd0, ExifTag::GPSInfo.code(), gps_offset);

            let mut tiff = Vec::new();
            tiff.extend_from_slice(b"II");
            push_u16(&mut tiff, 42);
            push_u32(&mut tiff, ifd0_offset);
            append_ifd(&mut tiff, &self.ifd0);
            append_ifd(&mut tiff, &self.exif);
            append_ifd(&mut tiff, &self.gps);

            let mut app1 = b"Exif\0\0".to_vec();
            app1.extend(tiff);

            let mut jpeg = vec![0xff, 0xd8, 0xff, 0xe1];
            push_be_u16(&mut jpeg, (app1.len() + 2) as u16);
            jpeg.extend(app1);
            jpeg.extend_from_slice(&[0xff, 0xd9]);
            jpeg
        }
    }

    fn test_exif_jpeg_builder() -> TestExifJpegBuilder {
        TestExifJpegBuilder::new()
    }

    fn ifd_size(entries: &[IfdEntry]) -> u32 {
        2 + entries.len() as u32 * IFD_ENTRY_SIZE + 4 + overflow_size(entries)
    }

    fn overflow_size(entries: &[IfdEntry]) -> u32 {
        entries
            .iter()
            .filter(|entry| entry.value.len() > 4)
            .map(|entry| entry.value.len() as u32)
            .sum()
    }

    fn append_ifd(data: &mut Vec<u8>, entries: &[IfdEntry]) {
        let base_offset = data.len() as u32;
        let value_base = base_offset + 2 + entries.len() as u32 * IFD_ENTRY_SIZE + 4;
        let mut overflow: Vec<u8> = Vec::new();
        push_u16(data, entries.len() as u16);
        for entry in entries {
            push_u16(data, entry.tag);
            push_u16(data, entry.format);
            push_u32(data, entry.count);
            if entry.value.len() <= 4 {
                data.extend(&entry.value);
                data.resize(data.len() + (4 - entry.value.len()), 0);
            } else {
                push_u32(data, value_base + overflow.len() as u32);
                overflow.extend(&entry.value);
            }
        }
        push_u32(data, 0);
        data.extend(overflow);
    }

    fn ascii_entry(tag: u16, value: &str) -> IfdEntry {
        let mut bytes = value.as_bytes().to_vec();
        bytes.push(0);
        IfdEntry {
            tag,
            format: 2,
            count: bytes.len() as u32,
            value: bytes,
        }
    }

    fn rational_entry(tag: u16, numerator: u32, denominator: u32) -> IfdEntry {
        rational_array_entry(tag, &[(numerator, denominator)])
    }

    fn rational_array_entry(tag: u16, values: &[(u32, u32)]) -> IfdEntry {
        let mut value = Vec::new();
        for (numerator, denominator) in values {
            push_u32(&mut value, *numerator);
            push_u32(&mut value, *denominator);
        }
        IfdEntry {
            tag,
            format: 5,
            count: values.len() as u32,
            value,
        }
    }

    fn short_entry(tag: u16, value: u16) -> IfdEntry {
        let mut bytes = Vec::new();
        push_u16(&mut bytes, value);
        IfdEntry {
            tag,
            format: 3,
            count: 1,
            value: bytes,
        }
    }

    fn long_entry(tag: u16, value: u32) -> IfdEntry {
        let mut bytes = Vec::new();
        push_u32(&mut bytes, value);
        IfdEntry {
            tag,
            format: 4,
            count: 1,
            value: bytes,
        }
    }

    fn set_long_value(entries: &mut [IfdEntry], tag: u16, value: u32) {
        if let Some(entry) = entries.iter_mut().find(|entry| entry.tag == tag) {
            entry.value.clear();
            push_u32(&mut entry.value, value);
        }
    }

    fn push_u16(data: &mut Vec<u8>, value: u16) {
        data.extend_from_slice(&value.to_le_bytes());
    }

    fn push_u32(data: &mut Vec<u8>, value: u32) {
        data.extend_from_slice(&value.to_le_bytes());
    }

    fn push_be_u16(data: &mut Vec<u8>, value: u16) {
        data.extend_from_slice(&value.to_be_bytes());
    }

    fn push_be_u32(data: &mut Vec<u8>, value: u32) {
        data.extend_from_slice(&value.to_be_bytes());
    }

    fn append_box(data: &mut Vec<u8>, kind: &[u8; 4], payload: &[u8]) {
        push_be_u32(data, (payload.len() + 8) as u32);
        data.extend_from_slice(kind);
        data.extend_from_slice(payload);
    }

    fn jpeg_with_exif_metadata() -> Vec<u8> {
        test_exif_jpeg_builder()
            .ascii(0x010f, "Kiri Camera Co.")
            .ascii(0x0110, "KiriCam 1")
            .ascii(0x0131, "KiriOS Camera")
            .ascii(0x9003, "2026:05:31 12:34:56")
            .ascii(0xa434, "Kiri Prime 35mm")
            .rational(0x829a, 1, 125)
            .rational(0x829d, 56, 10)
            .short(0x8827, 400)
            .rational(0x920a, 35, 1)
            .gps_ref(0x0001, "N")
            .gps_rational_triplet(0x0002, [(37, 1), (46, 1), (2964, 100)])
            .gps_ref(0x0003, "W")
            .gps_rational_triplet(0x0004, [(122, 1), (25, 1), (984, 100)])
            .finish()
    }

    fn tiny_metadata_mp4() -> Vec<u8> {
        let mut ftyp_payload = Vec::new();
        ftyp_payload.extend_from_slice(b"isom");
        ftyp_payload.extend_from_slice(&[0; 4]);
        ftyp_payload.extend_from_slice(b"isomiso2mp41");

        let mut mvhd_payload = vec![0; 12];
        push_be_u32(&mut mvhd_payload, 1000);
        push_be_u32(&mut mvhd_payload, 1234);
        mvhd_payload.extend_from_slice(&[0; 80]);

        let mut tkhd_payload = vec![0, 0, 0, 7];
        tkhd_payload.extend_from_slice(&[0; 16]);
        push_be_u32(&mut tkhd_payload, 1234);
        tkhd_payload.extend_from_slice(&[0; 52]);
        push_be_u32(&mut tkhd_payload, 640 << 16);
        push_be_u32(&mut tkhd_payload, 360 << 16);

        let mut hdlr_payload = vec![0; 8];
        hdlr_payload.extend_from_slice(b"vide");
        hdlr_payload.extend_from_slice(&[0; 12]);
        let mut mdia_payload = Vec::new();
        append_box(&mut mdia_payload, b"hdlr", &hdlr_payload);

        let mut trak_payload = Vec::new();
        append_box(&mut trak_payload, b"tkhd", &tkhd_payload);
        append_box(&mut trak_payload, b"mdia", &mdia_payload);

        let mut moov_payload = Vec::new();
        append_box(&mut moov_payload, b"mvhd", &mvhd_payload);
        append_box(&mut moov_payload, b"trak", &trak_payload);

        let mut data = Vec::new();
        append_box(&mut data, b"ftyp", &ftyp_payload);
        append_box(&mut data, b"moov", &moov_payload);
        data
    }

    fn unique_temp_path(file_name: &str) -> std::path::PathBuf {
        let unique = SystemTime::now()
            .duration_since(SystemTime::UNIX_EPOCH)
            .expect("system time should be after UNIX epoch")
            .as_nanos();
        std::env::temp_dir().join(format!(
            "kiriview-{}-{unique}-{file_name}",
            std::process::id()
        ))
    }

    #[test]
    fn invalid_image_bytes_return_empty_metadata() {
        let metadata = parse_image_metadata(b"not an image");

        assert!(metadata.is_empty());
    }

    #[test]
    fn jpeg_exif_camera_fields_are_curated() {
        let metadata = parse_image_metadata(&jpeg_with_exif_metadata());

        assert_eq!(metadata.camera_make, "Kiri Camera Co.");
        assert_eq!(metadata.camera_model, "KiriCam 1");
        assert_eq!(metadata.lens, "Kiri Prime 35mm");
        assert_eq!(metadata.exposure, "1/125 s at f/5.6");
        assert_eq!(metadata.iso, "400");
        assert_eq!(metadata.focal_length, "35 mm");
        assert_eq!(metadata.software, "KiriOS Camera");
    }

    #[test]
    fn jpeg_exif_gps_and_date_fields_are_curated() {
        let metadata = parse_image_metadata(&jpeg_with_exif_metadata());

        assert_eq!(metadata.taken, "2026-05-31 12:34:56");
        assert_eq!(metadata.location, "37.7749, -122.4194");
    }

    #[test]
    fn advanced_rows_skip_curated_and_empty_values() {
        let metadata = parse_image_metadata(&jpeg_with_exif_metadata());

        assert!(
            metadata
                .advanced_rows
                .iter()
                .all(|row| !row.value.is_empty())
        );
        assert!(
            metadata
                .advanced_rows
                .iter()
                .all(|row| row.label != "Make" && row.label != "Model")
        );
    }

    #[test]
    fn missing_video_path_returns_empty_metadata() {
        let metadata = parse_path_metadata("/tmp/kiriview-missing-video.mp4");

        assert!(metadata.is_empty());
    }

    #[test]
    fn direct_video_track_metadata_is_curated() {
        let path = unique_temp_path("metadata.mp4");
        fs::write(&path, tiny_metadata_mp4()).expect("test mp4 should be written");

        let metadata = parse_path_metadata(path.to_str().expect("test path should be UTF-8"));
        let _ = fs::remove_file(path);

        assert_eq!(metadata.duration, "00:00:01.234");
        assert_eq!(metadata.frame_size, "640×360 px");
    }
}
