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

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustThumbnailOriginalIdentityMode {
        LocalPath = 0,
        NonFileUri = 1,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustThumbnailOriginalIdentity {
        mode: RustThumbnailOriginalIdentityMode,
        local_path_bytes: Vec<u8>,
        uri: String,
        mtime_seconds: i64,
        original_byte_size: i64,
        mime_type: String,
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

    #[derive(Debug, PartialEq, Eq)]
    struct RustThumbnailCacheInstallResult {
        success: bool,
        requested_bucket: RustThumbnailCacheBucket,
        installed_cache_path: String,
        error: String,
    }

    extern "Rust" {
        #[cxx_name = "rustLookupDisplayThumbnailRgba8"]
        fn rust_lookup_display_thumbnail_rgba8(
            local_path_bytes: &[u8],
            requested_bucket: RustThumbnailCacheBucket,
        ) -> RustThumbnailCacheLookupResult;

        #[cxx_name = "rustLookupDisplayThumbnailOriginalRgba8"]
        fn rust_lookup_display_thumbnail_original_rgba8(
            identity: &RustThumbnailOriginalIdentity,
            requested_bucket: RustThumbnailCacheBucket,
        ) -> RustThumbnailCacheLookupResult;

        #[cxx_name = "rustLookupDisplayThumbnailNonFileUriRgba8"]
        fn rust_lookup_display_thumbnail_non_file_uri_rgba8(
            uri: &str,
            mtime_seconds: i64,
            original_byte_size: i64,
            mime_type: &str,
            requested_bucket: RustThumbnailCacheBucket,
        ) -> RustThumbnailCacheLookupResult;

        #[cxx_name = "rustInstallDisplayThumbnailRgba8"]
        fn rust_install_display_thumbnail_rgba8(
            local_path_bytes: &[u8],
            requested_bucket: RustThumbnailCacheBucket,
            width: i32,
            height: i32,
            stride: i32,
            rgba8_pixels: &[u8],
        ) -> RustThumbnailCacheInstallResult;

        #[cxx_name = "rustInstallDisplayThumbnailOriginalRgba8"]
        fn rust_install_display_thumbnail_original_rgba8(
            identity: &RustThumbnailOriginalIdentity,
            requested_bucket: RustThumbnailCacheBucket,
            width: i32,
            height: i32,
            stride: i32,
            rgba8_pixels: &[u8],
        ) -> RustThumbnailCacheInstallResult;

        #[cxx_name = "rustInstallDisplayThumbnailNonFileUriRgba8"]
        #[allow(clippy::too_many_arguments)]
        fn rust_install_display_thumbnail_non_file_uri_rgba8(
            uri: &str,
            mtime_seconds: i64,
            original_byte_size: i64,
            mime_type: &str,
            requested_bucket: RustThumbnailCacheBucket,
            width: i32,
            height: i32,
            stride: i32,
            rgba8_pixels: &[u8],
        ) -> RustThumbnailCacheInstallResult;

        #[cxx_name = "rustVirtualThumbnailArchiveEntryCrc32Uri"]
        fn virtual_archive_entry_crc32_uri(crc32: u32, uncompressed_size: u64) -> String;
    }
}

use std::ffi::OsStr;
use std::path::PathBuf;

use std::os::unix::ffi::OsStrExt;

use ffi::{
    RustThumbnailCacheBucket, RustThumbnailCacheInstallResult, RustThumbnailCacheLookupResult,
    RustThumbnailCacheLookupStatus, RustThumbnailOriginalIdentity,
    RustThumbnailOriginalIdentityMode,
};
use xdg_thumbnail::{
    DisplayThumbnailRgba8LookupEntryParts, OwnedRawThumbnailImage, PersonalCacheRoot,
    PersonalOriginalIdentity, PersonalOriginalUri, PersonalThumbnailLookup,
    PersonalThumbnailRawInstallRequest, RawThumbnailPixelFormat, ReadablePersonalOriginalIdentity,
    ThumbnailSize, UnixMtimeSeconds,
};

impl RustThumbnailOriginalIdentity {
    fn local_path(local_path_bytes: &[u8]) -> Self {
        Self {
            mode: RustThumbnailOriginalIdentityMode::LocalPath,
            local_path_bytes: local_path_bytes.to_vec(),
            uri: String::new(),
            mtime_seconds: 0,
            original_byte_size: -1,
            mime_type: String::new(),
        }
    }

    #[cfg(test)]
    fn non_file_uri(
        uri: impl Into<String>,
        mtime_seconds: i64,
        original_byte_size: u64,
        mime_type: impl Into<String>,
    ) -> Self {
        Self {
            mode: RustThumbnailOriginalIdentityMode::NonFileUri,
            local_path_bytes: Vec::new(),
            uri: uri.into(),
            mtime_seconds,
            original_byte_size: i64::try_from(original_byte_size).unwrap_or(i64::MAX),
            mime_type: mime_type.into(),
        }
    }
}

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
    lookup_display_thumbnail_rgba8_at_root(
        root,
        &RustThumbnailOriginalIdentity::local_path(local_path_bytes),
        requested_size,
    )
}

fn rust_lookup_display_thumbnail_original_rgba8(
    identity: &RustThumbnailOriginalIdentity,
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
    lookup_display_thumbnail_rgba8_at_root(root, identity, requested_size)
}

fn rust_lookup_display_thumbnail_non_file_uri_rgba8(
    uri: &str,
    mtime_seconds: i64,
    original_byte_size: i64,
    mime_type: &str,
    requested_bucket: RustThumbnailCacheBucket,
) -> RustThumbnailCacheLookupResult {
    rust_lookup_display_thumbnail_original_rgba8(
        &RustThumbnailOriginalIdentity {
            mode: RustThumbnailOriginalIdentityMode::NonFileUri,
            local_path_bytes: Vec::new(),
            uri: uri.to_owned(),
            mtime_seconds,
            original_byte_size,
            mime_type: mime_type.to_owned(),
        },
        requested_bucket,
    )
}

fn lookup_display_thumbnail_rgba8_at_root(
    root: PersonalCacheRoot,
    identity: &RustThumbnailOriginalIdentity,
    requested_size: ThumbnailSize,
) -> RustThumbnailCacheLookupResult {
    let requested_bucket = bucket_from_thumbnail_size(requested_size);
    let original = match readable_original_identity(identity) {
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

fn rust_install_display_thumbnail_rgba8(
    local_path_bytes: &[u8],
    requested_bucket: RustThumbnailCacheBucket,
    width: i32,
    height: i32,
    stride: i32,
    rgba8_pixels: &[u8],
) -> RustThumbnailCacheInstallResult {
    let Some(requested_size) = thumbnail_size_from_bucket(requested_bucket) else {
        return install_failed_result(
            requested_bucket,
            "thumbnail cache install requires a concrete size bucket",
        );
    };

    let root = match PersonalCacheRoot::resolve_from_env() {
        Ok(root) => root,
        Err(error) => return install_failed_result(requested_bucket, error),
    };
    install_display_thumbnail_rgba8_at_root(
        root,
        &RustThumbnailOriginalIdentity::local_path(local_path_bytes),
        requested_size,
        width,
        height,
        stride,
        rgba8_pixels,
    )
}

fn rust_install_display_thumbnail_original_rgba8(
    identity: &RustThumbnailOriginalIdentity,
    requested_bucket: RustThumbnailCacheBucket,
    width: i32,
    height: i32,
    stride: i32,
    rgba8_pixels: &[u8],
) -> RustThumbnailCacheInstallResult {
    let Some(requested_size) = thumbnail_size_from_bucket(requested_bucket) else {
        return install_failed_result(
            requested_bucket,
            "thumbnail cache install requires a concrete size bucket",
        );
    };

    let root = match PersonalCacheRoot::resolve_from_env() {
        Ok(root) => root,
        Err(error) => return install_failed_result(requested_bucket, error),
    };
    install_display_thumbnail_rgba8_at_root(
        root,
        identity,
        requested_size,
        width,
        height,
        stride,
        rgba8_pixels,
    )
}

#[allow(clippy::too_many_arguments)]
fn rust_install_display_thumbnail_non_file_uri_rgba8(
    uri: &str,
    mtime_seconds: i64,
    original_byte_size: i64,
    mime_type: &str,
    requested_bucket: RustThumbnailCacheBucket,
    width: i32,
    height: i32,
    stride: i32,
    rgba8_pixels: &[u8],
) -> RustThumbnailCacheInstallResult {
    rust_install_display_thumbnail_original_rgba8(
        &RustThumbnailOriginalIdentity {
            mode: RustThumbnailOriginalIdentityMode::NonFileUri,
            local_path_bytes: Vec::new(),
            uri: uri.to_owned(),
            mtime_seconds,
            original_byte_size,
            mime_type: mime_type.to_owned(),
        },
        requested_bucket,
        width,
        height,
        stride,
        rgba8_pixels,
    )
}

fn install_display_thumbnail_rgba8_at_root(
    root: PersonalCacheRoot,
    identity: &RustThumbnailOriginalIdentity,
    requested_size: ThumbnailSize,
    width: i32,
    height: i32,
    stride: i32,
    rgba8_pixels: &[u8],
) -> RustThumbnailCacheInstallResult {
    let requested_bucket = bucket_from_thumbnail_size(requested_size);
    let original = match readable_original_identity(identity) {
        Ok(original) => original,
        Err(error) => return install_failed_result(requested_bucket, error),
    };
    let width = match u32::try_from(width) {
        Ok(width) => width,
        Err(error) => return install_failed_result(requested_bucket, error),
    };
    let height = match u32::try_from(height) {
        Ok(height) => height,
        Err(error) => return install_failed_result(requested_bucket, error),
    };
    let stride = match usize::try_from(stride) {
        Ok(stride) => stride,
        Err(error) => return install_failed_result(requested_bucket, error),
    };
    let image = match OwnedRawThumbnailImage::new(
        width,
        height,
        stride,
        RawThumbnailPixelFormat::Rgba8,
        rgba8_pixels.to_vec(),
    ) {
        Ok(image) => image,
        Err(error) => return install_failed_result(requested_bucket, error),
    };
    let request = PersonalThumbnailRawInstallRequest::new(root, original, requested_size, image);
    match request.install_path() {
        Ok(installed) => RustThumbnailCacheInstallResult {
            success: true,
            requested_bucket,
            installed_cache_path: installed.path().to_string_lossy().into_owned(),
            error: String::new(),
        },
        Err(error) => install_failed_result(requested_bucket, error),
    }
}

fn readable_original_identity(
    identity: &RustThumbnailOriginalIdentity,
) -> xdg_thumbnail::Result<ReadablePersonalOriginalIdentity> {
    match identity.mode {
        RustThumbnailOriginalIdentityMode::LocalPath => {
            let local_path = PathBuf::from(OsStr::from_bytes(&identity.local_path_bytes));
            ReadablePersonalOriginalIdentity::from_local_path(&local_path)
        }
        RustThumbnailOriginalIdentityMode::NonFileUri => {
            let uri = PersonalOriginalUri::from_non_file_uri(&identity.uri)?;
            let mtime = UnixMtimeSeconds::try_from_i64(identity.mtime_seconds)?;
            let mut original = PersonalOriginalIdentity::new(uri, mtime);
            if identity.original_byte_size >= 0 {
                original = original.with_original_byte_size(identity.original_byte_size as u64);
            }
            if !identity.mime_type.is_empty() {
                original = original.with_mime_type(identity.mime_type.clone())?;
            }
            Ok(ReadablePersonalOriginalIdentity::assume_readable(original))
        }
        _ => unreachable!("unknown thumbnail original identity mode"),
    }
}

fn virtual_archive_entry_crc32_uri(crc32: u32, uncompressed_size: u64) -> String {
    format!("x-kiriview://thumbnail/archive-entry/v1/crc32/{crc32:08x}/{uncompressed_size}")
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

fn install_failed_result(
    requested_bucket: RustThumbnailCacheBucket,
    error: impl ToString,
) -> RustThumbnailCacheInstallResult {
    RustThumbnailCacheInstallResult {
        success: false,
        requested_bucket,
        installed_cache_path: String::new(),
        error: error.to_string(),
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
    use std::os::unix::fs::PermissionsExt;

    use xdg_thumbnail::RawThumbnailImage;

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
            &RustThumbnailOriginalIdentity::local_path(missing.as_os_str().as_bytes()),
            ThumbnailSize::Normal,
        );

        assert_eq!(result.status, RustThumbnailCacheLookupStatus::Failed);
        assert!(!result.error.is_empty());
    }

    #[test]
    fn raw_install_success_creates_lookup_readable_cache_entry() {
        let fixture = Fixture::new();
        let pixels = [50, 60, 70, 255, 80, 90, 100, 255];

        let install = fixture.install_via_bridge(ThumbnailSize::Normal, 2, 1, 8, &pixels);

        assert!(install.success);
        assert_eq!(install.requested_bucket, RustThumbnailCacheBucket::Normal);
        assert!(install.installed_cache_path.contains("/normal/"));

        let lookup = fixture.lookup(ThumbnailSize::Normal);
        assert_eq!(lookup.status, RustThumbnailCacheLookupStatus::Ready);
        assert_eq!(lookup.width, 2);
        assert_eq!(lookup.height, 1);
        assert_eq!(lookup.stride, 8);
        assert_eq!(lookup.pixels, pixels);
    }

    #[test]
    fn raw_install_rejects_invalid_bucket() {
        let fixture = Fixture::new();

        let install = install_display_thumbnail_rgba8_at_root(
            fixture.root.clone(),
            &RustThumbnailOriginalIdentity::local_path(
                fixture.original_path.as_os_str().as_bytes(),
            ),
            ThumbnailSize::Normal,
            1,
            1,
            4,
            &[1, 2, 3, 255],
        );
        assert!(install.success);

        let failed = rust_install_display_thumbnail_rgba8(
            fixture.original_path.as_os_str().as_bytes(),
            RustThumbnailCacheBucket::None,
            1,
            1,
            4,
            &[1, 2, 3, 255],
        );
        assert!(!failed.success);
        assert!(!failed.error.is_empty());
    }

    #[test]
    fn raw_install_rejects_invalid_dimensions_stride_and_buffer() {
        let fixture = Fixture::new();

        let zero_width = fixture.install_via_bridge(ThumbnailSize::Normal, 0, 1, 4, &[0, 0, 0, 0]);
        assert!(!zero_width.success);

        let short_stride =
            fixture.install_via_bridge(ThumbnailSize::Normal, 2, 1, 4, &[0, 0, 0, 0]);
        assert!(!short_stride.success);

        let short_buffer =
            fixture.install_via_bridge(ThumbnailSize::Normal, 2, 2, 8, &[0, 0, 0, 0]);
        assert!(!short_buffer.success);
    }

    #[test]
    fn raw_install_rejects_unreadable_or_nonexistent_original() {
        let temp = tempfile::TempDir::new().unwrap();
        let root = PersonalCacheRoot::new(temp.path().join("thumbnails")).unwrap();
        let missing = temp.path().join("missing.png");

        let missing_result = install_display_thumbnail_rgba8_at_root(
            root.clone(),
            &RustThumbnailOriginalIdentity::local_path(missing.as_os_str().as_bytes()),
            ThumbnailSize::Normal,
            1,
            1,
            4,
            &[1, 2, 3, 255],
        );
        assert!(!missing_result.success);

        let unreadable = temp.path().join("unreadable.png");
        fs::write(&unreadable, b"source").unwrap();
        let mut permissions = fs::metadata(&unreadable).unwrap().permissions();
        permissions.set_mode(0o0);
        fs::set_permissions(&unreadable, permissions).unwrap();

        let unreadable_result = install_display_thumbnail_rgba8_at_root(
            root,
            &RustThumbnailOriginalIdentity::local_path(unreadable.as_os_str().as_bytes()),
            ThumbnailSize::Normal,
            1,
            1,
            4,
            &[1, 2, 3, 255],
        );
        assert!(!unreadable_result.success);
    }

    #[test]
    fn explicit_non_file_uri_install_is_lookup_readable() {
        let temp = tempfile::TempDir::new().unwrap();
        let root = PersonalCacheRoot::new(temp.path().join("thumbnails")).unwrap();
        let original = virtual_original(
            "x-kiriview://thumbnail/archive-entry/v1/crc32/7a6c86f1/4",
            4,
        );
        let pixels = [1, 2, 3, 255];

        let install = install_display_thumbnail_rgba8_at_root(
            root.clone(),
            &original,
            ThumbnailSize::Normal,
            1,
            1,
            4,
            &pixels,
        );
        assert!(install.success);

        let lookup = lookup_display_thumbnail_rgba8_at_root(root, &original, ThumbnailSize::Normal);
        assert_eq!(lookup.status, RustThumbnailCacheLookupStatus::Ready);
        assert_eq!(lookup.pixels, pixels);
    }

    #[test]
    fn archive_entry_crc32_virtual_uri_uses_fixed_lower_hex_and_size() {
        assert_eq!(
            virtual_archive_entry_crc32_uri(0x0000000a, 16),
            "x-kiriview://thumbnail/archive-entry/v1/crc32/0000000a/16"
        );
        assert_eq!(
            virtual_archive_entry_crc32_uri(0x7a6c86f1, 3),
            "x-kiriview://thumbnail/archive-entry/v1/crc32/7a6c86f1/3"
        );
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
                &RustThumbnailOriginalIdentity::local_path(
                    self.original_path.as_os_str().as_bytes(),
                ),
                size,
            )
        }

        fn install_via_bridge(
            &self,
            size: ThumbnailSize,
            width: i32,
            height: i32,
            stride: i32,
            pixels: &[u8],
        ) -> RustThumbnailCacheInstallResult {
            let _keep_temp_alive = self.temp.path();
            install_display_thumbnail_rgba8_at_root(
                self.root.clone(),
                &RustThumbnailOriginalIdentity::local_path(
                    self.original_path.as_os_str().as_bytes(),
                ),
                size,
                width,
                height,
                stride,
                pixels,
            )
        }
    }

    fn virtual_original(uri: &str, byte_size: u64) -> RustThumbnailOriginalIdentity {
        RustThumbnailOriginalIdentity::non_file_uri(uri, 0, byte_size, "image/png")
    }
}
