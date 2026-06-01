// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustThumbnailCacheLookupStatus {
        Ready = 0,
        Missing = 1,
        Invalid = 2,
        Failed = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustThumbnailCacheBucket {
        None = 0,
        Normal = 1,
        Large = 2,
        XLarge = 3,
        XxLarge = 4,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustThumbnailCacheLookupResult {
        status: RustThumbnailCacheLookupStatus,
        pixels: Vec<u8>,
        width: i32,
        height: i32,
        stride: i32,
        requested_bucket: RustThumbnailCacheBucket,
        source_bucket: RustThumbnailCacheBucket,
        source_cache_path: String,
        error: String,
    }

    extern "Rust" {
        #[cxx_name = "rustLookupDisplayThumbnailRgba8"]
        fn rust_lookup_display_thumbnail_rgba8(
            local_path_bytes: &[u8],
            requested_bucket: RustThumbnailCacheBucket,
        ) -> RustThumbnailCacheLookupResult;
    }
}

use std::ffi::OsStr;
use std::path::PathBuf;

use std::os::unix::ffi::OsStrExt;

use ffi::{
    RustThumbnailCacheBucket, RustThumbnailCacheLookupResult, RustThumbnailCacheLookupStatus,
};
use xdg_thumbnail::{
    DisplayThumbnailRgba8LookupEntryParts, PersonalCacheRoot, PersonalThumbnailLookup,
    ReadablePersonalOriginalIdentity, ThumbnailSize,
};

fn rust_lookup_display_thumbnail_rgba8(
    local_path_bytes: &[u8],
    requested_bucket: RustThumbnailCacheBucket,
) -> RustThumbnailCacheLookupResult {
    let Some(requested_size) = thumbnail_size_from_bucket(requested_bucket) else {
        return failed_result(
            requested_bucket,
            "thumbnail cache lookup requires a concrete size bucket",
        );
    };

    let root = match PersonalCacheRoot::resolve_from_env() {
        Ok(root) => root,
        Err(error) => return failed_result(requested_bucket, error),
    };
    lookup_display_thumbnail_rgba8_at_root(root, local_path_bytes, requested_size)
}

fn lookup_display_thumbnail_rgba8_at_root(
    root: PersonalCacheRoot,
    local_path_bytes: &[u8],
    requested_size: ThumbnailSize,
) -> RustThumbnailCacheLookupResult {
    let requested_bucket = bucket_from_thumbnail_size(requested_size);
    let local_path = PathBuf::from(OsStr::from_bytes(local_path_bytes));
    let original = match ReadablePersonalOriginalIdentity::from_local_path(&local_path) {
        Ok(original) => original,
        Err(error) => return failed_result(requested_bucket, error),
    };

    match root.lookup_display_thumbnail_rgba8(&original, requested_size) {
        Ok(PersonalThumbnailLookup::Valid(entry)) => ready_result(entry.into_parts()),
        Ok(PersonalThumbnailLookup::Missing) => missing_result(requested_bucket),
        Ok(PersonalThumbnailLookup::Invalid(problems)) => invalid_result(
            requested_bucket,
            format!("invalid thumbnail cache entry: {problems:?}"),
        ),
        Err(error) => failed_result(requested_bucket, error),
    }
}

fn ready_result(parts: DisplayThumbnailRgba8LookupEntryParts) -> RustThumbnailCacheLookupResult {
    RustThumbnailCacheLookupResult {
        status: RustThumbnailCacheLookupStatus::Ready,
        pixels: parts.pixels,
        width: i32::try_from(parts.width).unwrap_or(0),
        height: i32::try_from(parts.height).unwrap_or(0),
        stride: i32::try_from(parts.stride).unwrap_or(0),
        requested_bucket: bucket_from_thumbnail_size(parts.requested_size),
        source_bucket: bucket_from_thumbnail_size(parts.source_size),
        source_cache_path: parts.source_path.to_string_lossy().into_owned(),
        error: String::new(),
    }
}

fn missing_result(requested_bucket: RustThumbnailCacheBucket) -> RustThumbnailCacheLookupResult {
    RustThumbnailCacheLookupResult {
        status: RustThumbnailCacheLookupStatus::Missing,
        pixels: Vec::new(),
        width: 0,
        height: 0,
        stride: 0,
        requested_bucket,
        source_bucket: RustThumbnailCacheBucket::None,
        source_cache_path: String::new(),
        error: String::new(),
    }
}

fn invalid_result(
    requested_bucket: RustThumbnailCacheBucket,
    error: impl ToString,
) -> RustThumbnailCacheLookupResult {
    RustThumbnailCacheLookupResult {
        status: RustThumbnailCacheLookupStatus::Invalid,
        error: error.to_string(),
        ..missing_result(requested_bucket)
    }
}

fn failed_result(
    requested_bucket: RustThumbnailCacheBucket,
    error: impl ToString,
) -> RustThumbnailCacheLookupResult {
    RustThumbnailCacheLookupResult {
        status: RustThumbnailCacheLookupStatus::Failed,
        error: error.to_string(),
        ..missing_result(requested_bucket)
    }
}

fn thumbnail_size_from_bucket(bucket: RustThumbnailCacheBucket) -> Option<ThumbnailSize> {
    match bucket {
        RustThumbnailCacheBucket::None => None,
        RustThumbnailCacheBucket::Normal => Some(ThumbnailSize::Normal),
        RustThumbnailCacheBucket::Large => Some(ThumbnailSize::Large),
        RustThumbnailCacheBucket::XLarge => Some(ThumbnailSize::XLarge),
        RustThumbnailCacheBucket::XxLarge => Some(ThumbnailSize::XxLarge),
        _ => None,
    }
}

fn bucket_from_thumbnail_size(size: ThumbnailSize) -> RustThumbnailCacheBucket {
    match size {
        ThumbnailSize::Normal => RustThumbnailCacheBucket::Normal,
        ThumbnailSize::Large => RustThumbnailCacheBucket::Large,
        ThumbnailSize::XLarge => RustThumbnailCacheBucket::XLarge,
        ThumbnailSize::XxLarge => RustThumbnailCacheBucket::XxLarge,
        _ => RustThumbnailCacheBucket::None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    use xdg_thumbnail::{RawThumbnailImage, RawThumbnailPixelFormat};

    #[test]
    fn bucket_mapping_matches_freedesktop_sizes() {
        assert_eq!(
            thumbnail_size_from_bucket(RustThumbnailCacheBucket::Normal),
            Some(ThumbnailSize::Normal)
        );
        assert_eq!(
            thumbnail_size_from_bucket(RustThumbnailCacheBucket::Large),
            Some(ThumbnailSize::Large)
        );
        assert_eq!(
            thumbnail_size_from_bucket(RustThumbnailCacheBucket::XLarge),
            Some(ThumbnailSize::XLarge)
        );
        assert_eq!(
            thumbnail_size_from_bucket(RustThumbnailCacheBucket::XxLarge),
            Some(ThumbnailSize::XxLarge)
        );
        assert_eq!(
            thumbnail_size_from_bucket(RustThumbnailCacheBucket::None),
            None
        );
    }

    #[test]
    fn exact_cache_hit_returns_display_rgba8_pixels() {
        let fixture = Fixture::new();
        let pixels = [10, 20, 30, 255, 40, 50, 60, 127];
        fixture.install(ThumbnailSize::Normal, 2, 1, &pixels);

        let result = fixture.lookup(ThumbnailSize::Normal);

        assert_eq!(result.status, RustThumbnailCacheLookupStatus::Ready);
        assert_eq!(result.width, 2);
        assert_eq!(result.height, 1);
        assert_eq!(result.stride, 8);
        assert_eq!(result.pixels, pixels);
        assert_eq!(result.requested_bucket, RustThumbnailCacheBucket::Normal);
        assert_eq!(result.source_bucket, RustThumbnailCacheBucket::Normal);
        assert!(result.source_cache_path.contains("/normal/"));
    }

    #[test]
    fn larger_cache_hit_is_accepted_for_display() {
        let fixture = Fixture::new();
        fixture.install(ThumbnailSize::Large, 2, 1, &[1, 2, 3, 255, 4, 5, 6, 255]);

        let result = fixture.lookup(ThumbnailSize::Normal);

        assert_eq!(result.status, RustThumbnailCacheLookupStatus::Ready);
        assert_eq!(result.requested_bucket, RustThumbnailCacheBucket::Normal);
        assert_eq!(result.source_bucket, RustThumbnailCacheBucket::Large);
        assert!(result.source_cache_path.contains("/large/"));
    }

    #[test]
    fn missing_cache_reports_missing() {
        let fixture = Fixture::new();

        let result = fixture.lookup(ThumbnailSize::Normal);

        assert_eq!(result.status, RustThumbnailCacheLookupStatus::Missing);
        assert!(result.pixels.is_empty());
    }

    #[test]
    fn invalid_cache_reports_invalid() {
        let fixture = Fixture::new();
        let path = fixture.root.cache_entry_path(
            fixture.original.identity().uri(),
            &xdg_thumbnail::CacheNamespace::Size(ThumbnailSize::Normal),
        );
        fs::create_dir_all(path.parent().unwrap()).unwrap();
        fs::write(path, b"not a png").unwrap();

        let result = fixture.lookup(ThumbnailSize::Normal);

        assert_eq!(result.status, RustThumbnailCacheLookupStatus::Invalid);
        assert!(!result.error.is_empty());
    }

    #[test]
    fn nonexistent_local_path_reports_failure() {
        let temp = tempfile::TempDir::new().unwrap();
        let root = PersonalCacheRoot::new(temp.path().join("thumbnails")).unwrap();
        let missing = temp.path().join("missing.png");

        let result = lookup_display_thumbnail_rgba8_at_root(
            root,
            missing.as_os_str().as_bytes(),
            ThumbnailSize::Normal,
        );

        assert_eq!(result.status, RustThumbnailCacheLookupStatus::Failed);
        assert!(!result.error.is_empty());
    }

    struct Fixture {
        temp: tempfile::TempDir,
        root: PersonalCacheRoot,
        original_path: PathBuf,
        original: ReadablePersonalOriginalIdentity,
    }

    impl Fixture {
        fn new() -> Self {
            let temp = tempfile::TempDir::new().unwrap();
            let original_path = temp.path().join("photo.png");
            fs::write(&original_path, b"source").unwrap();
            let root = PersonalCacheRoot::new(temp.path().join("thumbnails")).unwrap();
            let original =
                ReadablePersonalOriginalIdentity::from_local_path(&original_path).unwrap();
            Self {
                temp,
                root,
                original_path,
                original,
            }
        }

        fn install(&self, size: ThumbnailSize, width: u32, height: u32, pixels: &[u8]) {
            let image = RawThumbnailImage::new(
                width,
                height,
                usize::try_from(width).unwrap() * 4,
                RawThumbnailPixelFormat::Rgba8,
                pixels,
            )
            .unwrap();
            self.root
                .install_raw_thumbnail_returning_path(&self.original, size, image)
                .unwrap();
        }

        fn lookup(&self, size: ThumbnailSize) -> RustThumbnailCacheLookupResult {
            let _keep_temp_alive = self.temp.path();
            lookup_display_thumbnail_rgba8_at_root(
                self.root.clone(),
                self.original_path.as_os_str().as_bytes(),
                size,
            )
        }
    }
}
