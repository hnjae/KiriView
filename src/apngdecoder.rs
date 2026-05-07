// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView decodes APNG here because the Qt PNG image
// plugin currently exposes affected APNG files as still PNG images instead of
// their authored animations. Remove this module and route APNG through
// QImageReader once Qt's PNG stack reliably exposes APNG frames, delays,
// blend/disposal operations, and loop counts.

mod parser;
mod render;

use cxx_qt_lib::QByteArray;
use parser::parse_apng;
use render::render_frames;
use std::cmp;

const RGBA_BYTES_PER_PIXEL: usize = 4;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qbytearray.h");

        #[namespace = ""]
        type QByteArray = cxx_qt_lib::QByteArray;
    }

    enum RustApngDecodeStatus {
        NotApng = 0,
        Success = 1,
        Error = 2,
    }

    enum RustApngErrorKind {
        NoError = 0,
        Png = 1,
        Apng = 2,
        FrameData = 3,
    }

    extern "Rust" {
        type RustApngDecodeResult;

        #[cxx_name = "decodeApngAnimationRust"]
        fn decode_apng_animation_rust(data: &QByteArray) -> Box<RustApngDecodeResult>;

        #[cxx_name = "rustApngStatus"]
        fn rust_apng_status(result: &RustApngDecodeResult) -> RustApngDecodeStatus;

        #[cxx_name = "rustApngErrorKind"]
        fn rust_apng_error_kind(result: &RustApngDecodeResult) -> RustApngErrorKind;

        #[cxx_name = "rustApngWidth"]
        fn rust_apng_width(result: &RustApngDecodeResult) -> u32;

        #[cxx_name = "rustApngHeight"]
        fn rust_apng_height(result: &RustApngDecodeResult) -> u32;

        #[cxx_name = "rustApngLoopCount"]
        fn rust_apng_loop_count(result: &RustApngDecodeResult) -> i32;

        #[cxx_name = "rustApngFrameCount"]
        fn rust_apng_frame_count(result: &RustApngDecodeResult) -> usize;

        #[cxx_name = "rustApngFrameDelay"]
        fn rust_apng_frame_delay(result: &RustApngDecodeResult, index: usize) -> i32;

        #[cxx_name = "rustApngFrameBytes"]
        fn rust_apng_frame_bytes(result: &RustApngDecodeResult, index: usize) -> &[u8];
    }
}

use ffi::{RustApngDecodeStatus, RustApngErrorKind};

pub struct RustApngDecodeResult {
    status: RustApngDecodeStatus,
    error_kind: RustApngErrorKind,
    animation: Option<DecodedApngAnimation>,
}

struct DecodedApngAnimation {
    width: u32,
    height: u32,
    loop_count: i32,
    frames: Vec<DecodedApngFrame>,
}

struct DecodedApngFrame {
    rgba: Vec<u8>,
    delay: i32,
}

enum DecodeOutcome {
    NotApng,
    Success(DecodedApngAnimation),
    Error(RustApngErrorKind),
}

fn decode_apng_animation_rust(data: &QByteArray) -> Box<RustApngDecodeResult> {
    Box::new(match decode_apng(data.as_slice()) {
        DecodeOutcome::NotApng => RustApngDecodeResult {
            status: RustApngDecodeStatus::NotApng,
            error_kind: RustApngErrorKind::NoError,
            animation: None,
        },
        DecodeOutcome::Success(animation) => RustApngDecodeResult {
            status: RustApngDecodeStatus::Success,
            error_kind: RustApngErrorKind::NoError,
            animation: Some(animation),
        },
        DecodeOutcome::Error(error_kind) => RustApngDecodeResult {
            status: RustApngDecodeStatus::Error,
            error_kind,
            animation: None,
        },
    })
}

fn rust_apng_status(result: &RustApngDecodeResult) -> RustApngDecodeStatus {
    result.status
}

fn rust_apng_error_kind(result: &RustApngDecodeResult) -> RustApngErrorKind {
    result.error_kind
}

fn rust_apng_width(result: &RustApngDecodeResult) -> u32 {
    result
        .animation
        .as_ref()
        .map_or(0, |animation| animation.width)
}

fn rust_apng_height(result: &RustApngDecodeResult) -> u32 {
    result
        .animation
        .as_ref()
        .map_or(0, |animation| animation.height)
}

fn rust_apng_loop_count(result: &RustApngDecodeResult) -> i32 {
    result
        .animation
        .as_ref()
        .map_or(0, |animation| animation.loop_count)
}

fn rust_apng_frame_count(result: &RustApngDecodeResult) -> usize {
    result
        .animation
        .as_ref()
        .map_or(0, |animation| animation.frames.len())
}

fn rust_apng_frame_delay(result: &RustApngDecodeResult, index: usize) -> i32 {
    result
        .animation
        .as_ref()
        .and_then(|animation| animation.frames.get(index))
        .map_or(0, |frame| frame.delay)
}

fn rust_apng_frame_bytes(result: &RustApngDecodeResult, index: usize) -> &[u8] {
    result
        .animation
        .as_ref()
        .and_then(|animation| animation.frames.get(index))
        .map_or(&[], |frame| frame.rgba.as_slice())
}

fn decode_apng(data: &[u8]) -> DecodeOutcome {
    let parsed = match parse_apng(data) {
        Ok(Some(parsed)) => parsed,
        Ok(None) => return DecodeOutcome::NotApng,
        Err(error_kind) => return DecodeOutcome::Error(error_kind),
    };

    let frames = match render_frames(&parsed) {
        Ok(frames) => frames,
        Err(error_kind) => return DecodeOutcome::Error(error_kind),
    };

    DecodeOutcome::Success(DecodedApngAnimation {
        width: parsed.canvas_width,
        height: parsed.canvas_height,
        loop_count: loop_count(parsed.play_count),
        frames,
    })
}

fn loop_count(play_count: u32) -> i32 {
    if play_count == 0 {
        return -1;
    }
    cmp::min(play_count - 1, i32::MAX as u32) as i32
}

fn rgba_len(
    width: u32,
    height: u32,
    error_kind: RustApngErrorKind,
) -> Result<usize, RustApngErrorKind> {
    let len = u64::from(width)
        .checked_mul(u64::from(height))
        .and_then(|pixels| pixels.checked_mul(RGBA_BYTES_PER_PIXEL as u64))
        .ok_or(error_kind)?;
    if len > isize::MAX as u64 {
        return Err(error_kind);
    }
    usize::try_from(len).map_err(|_| error_kind)
}

#[cfg(test)]
mod tests;
