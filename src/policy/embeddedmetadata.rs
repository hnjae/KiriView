// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cfg(test)]
mod tests {
    use super::*;

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
}
