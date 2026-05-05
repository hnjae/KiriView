// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView decodes APNG here because the Qt PNG image
// plugin currently exposes affected APNG files as still PNG images instead of
// their authored animations. Remove this module and route APNG through
// QImageReader once Qt's PNG stack reliably exposes APNG frames, delays,
// blend/disposal operations, and loop counts.

use cxx_qt_lib::QByteArray;
use png::{BitDepth, ColorType};
use std::cmp;
use std::io::Cursor;
use std::ops::Range;

const PNG_SIGNATURE: [u8; 8] = [0x89, b'P', b'N', b'G', 0x0d, 0x0a, 0x1a, 0x0a];
const APNG_DEFAULT_DELAY_DENOMINATOR: u16 = 100;
const MAX_QIMAGE_DIMENSION: u32 = i32::MAX as u32;
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

#[derive(Clone)]
struct PngChunk {
    kind: [u8; 4],
    data: Vec<u8>,
}

struct PngChunkView<'a> {
    kind: [u8; 4],
    data: &'a [u8],
    next_offset: usize,
}

#[derive(Clone, Copy)]
struct FrameControl {
    sequence_number: u32,
    width: u32,
    height: u32,
    x_offset: u32,
    y_offset: u32,
    delay_num: u16,
    delay_den: u16,
    dispose_op: DisposeOp,
    blend_op: BlendOp,
}

#[derive(Clone, Copy, Eq, PartialEq)]
enum DisposeOp {
    None,
    Background,
    Previous,
}

#[derive(Clone, Copy, Eq, PartialEq)]
enum BlendOp {
    Source,
    Over,
}

struct RawFrame {
    control: FrameControl,
    image_data: Vec<u8>,
    uses_default_image_data: bool,
}

struct ParsedApng {
    canvas_width: u32,
    canvas_height: u32,
    play_count: u32,
    ihdr_data: Vec<u8>,
    header_chunks: Vec<PngChunk>,
    frames: Vec<RawFrame>,
}

struct ApngParseState {
    seen_ihdr: bool,
    seen_actl: bool,
    seen_idat: bool,
    idat_chunks_open: bool,
    seen_iend: bool,
    expected_frame_count: u32,
    expected_sequence_number: u32,
    parsed: ParsedApng,
    current_frame: Option<RawFrame>,
}

impl ApngParseState {
    fn new() -> Self {
        Self {
            seen_ihdr: false,
            seen_actl: false,
            seen_idat: false,
            idat_chunks_open: false,
            seen_iend: false,
            expected_frame_count: 0,
            expected_sequence_number: 0,
            parsed: ParsedApng {
                canvas_width: 0,
                canvas_height: 0,
                play_count: 0,
                ihdr_data: Vec::new(),
                header_chunks: Vec::new(),
                frames: Vec::new(),
            },
            current_frame: None,
        }
    }

    fn is_complete(&self) -> bool {
        self.seen_iend
    }

    fn handle_chunk(&mut self, chunk: PngChunkView<'_>) -> Result<(), RustApngErrorKind> {
        let kind = chunk.kind;
        let chunk_data = chunk.data;

        if !self.seen_ihdr && kind != *b"IHDR" {
            return Err(RustApngErrorKind::Png);
        }

        match &kind {
            b"IHDR" => self.handle_ihdr(chunk_data),
            b"acTL" => self.handle_actl(chunk_data),
            b"fcTL" => self.handle_fctl(chunk_data),
            b"IDAT" => self.handle_idat(chunk_data),
            b"fdAT" => self.handle_fdat(chunk_data),
            b"IEND" => self.handle_iend(),
            _ => {
                self.handle_other_chunk(kind, chunk_data);
                Ok(())
            }
        }
    }

    fn finish(self) -> Result<Option<ParsedApng>, RustApngErrorKind> {
        if !self.seen_ihdr || !self.seen_iend {
            return Err(RustApngErrorKind::Png);
        }
        if !self.seen_actl {
            return Ok(None);
        }
        if !self.seen_idat
            || usize::try_from(self.expected_frame_count).ok() != Some(self.parsed.frames.len())
        {
            return Err(RustApngErrorKind::Apng);
        }

        Ok(Some(self.parsed))
    }

    fn handle_ihdr(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if self.seen_ihdr || !self.parsed.ihdr_data.is_empty() || chunk_data.len() != 13 {
            return Err(RustApngErrorKind::Png);
        }
        self.seen_ihdr = true;
        self.parsed.ihdr_data = chunk_data.to_vec();
        self.parsed.canvas_width = read_u32(chunk_data, 0).ok_or(RustApngErrorKind::Png)?;
        self.parsed.canvas_height = read_u32(chunk_data, 4).ok_or(RustApngErrorKind::Png)?;
        validate_dimensions(
            self.parsed.canvas_width,
            self.parsed.canvas_height,
            RustApngErrorKind::Png,
        )
    }

    fn handle_actl(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if !self.seen_ihdr || self.seen_actl || self.seen_idat || chunk_data.len() != 8 {
            return Err(RustApngErrorKind::Apng);
        }
        self.seen_actl = true;
        self.expected_frame_count = read_u32(chunk_data, 0).ok_or(RustApngErrorKind::Apng)?;
        self.parsed.play_count = read_u32(chunk_data, 4).ok_or(RustApngErrorKind::Apng)?;
        if self.expected_frame_count == 0 {
            return Err(RustApngErrorKind::Apng);
        }
        Ok(())
    }

    fn handle_fctl(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if !self.seen_actl {
            return Err(RustApngErrorKind::Apng);
        }
        self.finish_current_frame()?;
        let control = read_frame_control(chunk_data)?;
        if control.sequence_number != self.expected_sequence_number {
            return Err(RustApngErrorKind::Apng);
        }
        self.advance_sequence_number()?;
        validate_frame_bounds(
            &control,
            self.parsed.canvas_width,
            self.parsed.canvas_height,
        )?;
        self.current_frame = Some(RawFrame {
            control,
            image_data: Vec::new(),
            uses_default_image_data: !self.seen_idat,
        });
        self.idat_chunks_open = false;
        Ok(())
    }

    fn handle_idat(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if !self.seen_ihdr {
            return Err(RustApngErrorKind::Png);
        }
        if self.seen_idat && !self.idat_chunks_open {
            return Err(RustApngErrorKind::Png);
        }
        if self
            .current_frame
            .as_ref()
            .is_some_and(|frame| !frame.uses_default_image_data)
        {
            return Err(RustApngErrorKind::Apng);
        }

        self.seen_idat = true;
        self.idat_chunks_open = true;
        if let Some(frame) = self.current_frame.as_mut()
            && frame.uses_default_image_data
        {
            frame.image_data.extend_from_slice(chunk_data);
        }
        Ok(())
    }

    fn handle_fdat(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if !self.seen_actl || !self.seen_idat || chunk_data.len() < 4 {
            return Err(RustApngErrorKind::Apng);
        }
        let sequence_number = read_u32(chunk_data, 0).ok_or(RustApngErrorKind::Apng)?;
        if sequence_number != self.expected_sequence_number {
            return Err(RustApngErrorKind::Apng);
        }
        self.advance_sequence_number()?;
        let frame = self.current_frame.as_mut().ok_or(RustApngErrorKind::Apng)?;
        if frame.uses_default_image_data {
            return Err(RustApngErrorKind::Apng);
        }
        frame.image_data.extend_from_slice(&chunk_data[4..]);
        self.idat_chunks_open = false;
        Ok(())
    }

    fn handle_iend(&mut self) -> Result<(), RustApngErrorKind> {
        self.finish_current_frame()?;
        self.seen_iend = true;
        Ok(())
    }

    fn handle_other_chunk(&mut self, kind: [u8; 4], chunk_data: &[u8]) {
        if !self.seen_idat {
            self.parsed.header_chunks.push(PngChunk {
                kind,
                data: chunk_data.to_vec(),
            });
        } else {
            self.idat_chunks_open = false;
        }
    }

    fn finish_current_frame(&mut self) -> Result<(), RustApngErrorKind> {
        let Some(frame) = self.current_frame.take() else {
            return Ok(());
        };
        if frame.image_data.is_empty() {
            return Err(RustApngErrorKind::Apng);
        }
        self.parsed.frames.push(frame);
        Ok(())
    }

    fn advance_sequence_number(&mut self) -> Result<(), RustApngErrorKind> {
        self.expected_sequence_number = self
            .expected_sequence_number
            .checked_add(1)
            .ok_or(RustApngErrorKind::Apng)?;
        Ok(())
    }
}

#[derive(Clone, Copy)]
struct Rect {
    x: usize,
    y: usize,
    width: usize,
    height: usize,
}

#[derive(Clone, Copy)]
struct RgbaRegion {
    rect: Rect,
    row_len: usize,
    byte_len: usize,
}

impl RgbaRegion {
    fn new(rect: Rect) -> Result<Self, RustApngErrorKind> {
        let row_len = rect
            .width
            .checked_mul(RGBA_BYTES_PER_PIXEL)
            .ok_or(RustApngErrorKind::Apng)?;
        let byte_len = row_len
            .checked_mul(rect.height)
            .ok_or(RustApngErrorKind::Apng)?;

        Ok(Self {
            rect,
            row_len,
            byte_len,
        })
    }

    fn height(self) -> usize {
        self.rect.height
    }

    fn byte_len(self) -> usize {
        self.byte_len
    }

    fn validate_data_len(self, data: &[u8]) -> Result<(), RustApngErrorKind> {
        if data.len() != self.byte_len {
            return Err(RustApngErrorKind::Apng);
        }

        Ok(())
    }

    fn canvas_row<'a>(
        self,
        canvas: &'a [u8],
        canvas_width: usize,
        row: usize,
    ) -> Result<&'a [u8], RustApngErrorKind> {
        canvas
            .get(self.canvas_row_range(canvas_width, row)?)
            .ok_or(RustApngErrorKind::Apng)
    }

    fn canvas_row_mut<'a>(
        self,
        canvas: &'a mut [u8],
        canvas_width: usize,
        row: usize,
    ) -> Result<&'a mut [u8], RustApngErrorKind> {
        canvas
            .get_mut(self.canvas_row_range(canvas_width, row)?)
            .ok_or(RustApngErrorKind::Apng)
    }

    fn data_row<'a>(self, data: &'a [u8], row: usize) -> Result<&'a [u8], RustApngErrorKind> {
        data.get(self.region_row_range(row)?)
            .ok_or(RustApngErrorKind::Apng)
    }

    fn canvas_row_range(
        self,
        canvas_width: usize,
        row: usize,
    ) -> Result<Range<usize>, RustApngErrorKind> {
        if row >= self.rect.height {
            return Err(RustApngErrorKind::Apng);
        }

        let y = self
            .rect
            .y
            .checked_add(row)
            .ok_or(RustApngErrorKind::Apng)?;
        let start = canvas_offset(canvas_width, self.rect.x, y)?;
        let end = start
            .checked_add(self.row_len)
            .ok_or(RustApngErrorKind::Apng)?;
        Ok(start..end)
    }

    fn region_row_range(self, row: usize) -> Result<Range<usize>, RustApngErrorKind> {
        if row >= self.rect.height {
            return Err(RustApngErrorKind::Apng);
        }

        let start = row
            .checked_mul(self.row_len)
            .ok_or(RustApngErrorKind::Apng)?;
        let end = start
            .checked_add(self.row_len)
            .ok_or(RustApngErrorKind::Apng)?;
        Ok(start..end)
    }
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

fn parse_apng(data: &[u8]) -> Result<Option<ParsedApng>, RustApngErrorKind> {
    if !data.starts_with(&PNG_SIGNATURE) {
        return Ok(None);
    }

    let mut offset = PNG_SIGNATURE.len();
    let mut state = ApngParseState::new();

    while offset < data.len() && !state.is_complete() {
        let chunk = read_png_chunk(data, offset)?;
        offset = chunk.next_offset;
        state.handle_chunk(chunk)?;
    }

    state.finish()
}

fn read_frame_control(data: &[u8]) -> Result<FrameControl, RustApngErrorKind> {
    if data.len() != 26 {
        return Err(RustApngErrorKind::Apng);
    }

    let dispose_op = match data[24] {
        0 => DisposeOp::None,
        1 => DisposeOp::Background,
        2 => DisposeOp::Previous,
        _ => return Err(RustApngErrorKind::Apng),
    };
    let blend_op = match data[25] {
        0 => BlendOp::Source,
        1 => BlendOp::Over,
        _ => return Err(RustApngErrorKind::Apng),
    };

    let control = FrameControl {
        sequence_number: read_u32(data, 0).ok_or(RustApngErrorKind::Apng)?,
        width: read_u32(data, 4).ok_or(RustApngErrorKind::Apng)?,
        height: read_u32(data, 8).ok_or(RustApngErrorKind::Apng)?,
        x_offset: read_u32(data, 12).ok_or(RustApngErrorKind::Apng)?,
        y_offset: read_u32(data, 16).ok_or(RustApngErrorKind::Apng)?,
        delay_num: read_u16(data, 20).ok_or(RustApngErrorKind::Apng)?,
        delay_den: read_u16(data, 22).ok_or(RustApngErrorKind::Apng)?,
        dispose_op,
        blend_op,
    };
    validate_dimensions(control.width, control.height, RustApngErrorKind::Apng)?;
    Ok(control)
}

fn validate_frame_bounds(
    control: &FrameControl,
    canvas_width: u32,
    canvas_height: u32,
) -> Result<(), RustApngErrorKind> {
    let right = control
        .x_offset
        .checked_add(control.width)
        .ok_or(RustApngErrorKind::Apng)?;
    let bottom = control
        .y_offset
        .checked_add(control.height)
        .ok_or(RustApngErrorKind::Apng)?;
    if right > canvas_width || bottom > canvas_height {
        return Err(RustApngErrorKind::Apng);
    }
    Ok(())
}

fn validate_dimensions(
    width: u32,
    height: u32,
    error_kind: RustApngErrorKind,
) -> Result<(), RustApngErrorKind> {
    if width == 0 || height == 0 || width > MAX_QIMAGE_DIMENSION || height > MAX_QIMAGE_DIMENSION {
        return Err(error_kind);
    }
    rgba_len(width, height, error_kind)?;
    Ok(())
}

fn render_frames(parsed: &ParsedApng) -> Result<Vec<DecodedApngFrame>, RustApngErrorKind> {
    let canvas_len = rgba_len(
        parsed.canvas_width,
        parsed.canvas_height,
        RustApngErrorKind::Apng,
    )?;
    let mut canvas = vec![0; canvas_len];
    let canvas_width = usize::try_from(parsed.canvas_width).map_err(|_| RustApngErrorKind::Apng)?;
    let mut frames = Vec::with_capacity(parsed.frames.len());

    for (index, frame) in parsed.frames.iter().enumerate() {
        let mut decoded_frame = decode_frame_rgba(parsed, frame)?;
        premultiply_rgba(&mut decoded_frame);
        let rect = rect_from_frame(frame)?;
        let previous_frame_region = (frame.control.dispose_op == DisposeOp::Previous)
            .then(|| copy_region(&canvas, canvas_width, rect))
            .transpose()?;

        blend_frame(
            &mut canvas,
            canvas_width,
            rect,
            &decoded_frame,
            frame.control.blend_op,
        )?;
        frames.push(DecodedApngFrame {
            rgba: canvas.clone(),
            delay: frame_delay(&frame.control),
        });

        match frame.control.dispose_op {
            DisposeOp::None => {}
            DisposeOp::Background => clear_region(&mut canvas, canvas_width, rect)?,
            DisposeOp::Previous => {
                if index == 0 {
                    clear_region(&mut canvas, canvas_width, rect)?;
                } else if let Some(previous_frame_region) = previous_frame_region {
                    restore_region(&mut canvas, canvas_width, rect, &previous_frame_region)?;
                }
            }
        }
    }

    Ok(frames)
}

fn decode_frame_rgba(parsed: &ParsedApng, frame: &RawFrame) -> Result<Vec<u8>, RustApngErrorKind> {
    let png_data = frame_png_data(parsed, frame)?;
    let mut decoder = png::Decoder::new(Cursor::new(png_data));
    decoder.set_transformations(
        png::Transformations::normalize_to_color8() | png::Transformations::ALPHA,
    );
    let mut reader = decoder
        .read_info()
        .map_err(|_| RustApngErrorKind::FrameData)?;
    let output_buffer_size = reader
        .output_buffer_size()
        .ok_or(RustApngErrorKind::FrameData)?;
    let mut output = vec![0; output_buffer_size];
    let info = reader
        .next_frame(&mut output)
        .map_err(|_| RustApngErrorKind::FrameData)?;
    if info.width != frame.control.width || info.height != frame.control.height {
        return Err(RustApngErrorKind::Apng);
    }
    rgba_from_png_output(&output[..info.buffer_size()], &info)
}

fn rgba_from_png_output(
    output: &[u8],
    info: &png::OutputInfo,
) -> Result<Vec<u8>, RustApngErrorKind> {
    if info.bit_depth != BitDepth::Eight {
        return Err(RustApngErrorKind::FrameData);
    }

    let pixel_count = usize::try_from(info.width)
        .ok()
        .and_then(|width| {
            usize::try_from(info.height)
                .ok()
                .and_then(|height| width.checked_mul(height))
        })
        .ok_or(RustApngErrorKind::FrameData)?;

    match info.color_type {
        ColorType::Rgba => {
            if output.len()
                != pixel_count
                    .checked_mul(4)
                    .ok_or(RustApngErrorKind::FrameData)?
            {
                return Err(RustApngErrorKind::FrameData);
            }
            Ok(output.to_vec())
        }
        ColorType::Rgb => {
            if output.len()
                != pixel_count
                    .checked_mul(3)
                    .ok_or(RustApngErrorKind::FrameData)?
            {
                return Err(RustApngErrorKind::FrameData);
            }
            let mut rgba = Vec::with_capacity(pixel_count * 4);
            for pixel in output.chunks_exact(3) {
                rgba.extend_from_slice(&[pixel[0], pixel[1], pixel[2], 255]);
            }
            Ok(rgba)
        }
        ColorType::Grayscale => {
            if output.len() != pixel_count {
                return Err(RustApngErrorKind::FrameData);
            }
            let mut rgba = Vec::with_capacity(pixel_count * 4);
            for gray in output {
                rgba.extend_from_slice(&[*gray, *gray, *gray, 255]);
            }
            Ok(rgba)
        }
        ColorType::GrayscaleAlpha => {
            if output.len()
                != pixel_count
                    .checked_mul(2)
                    .ok_or(RustApngErrorKind::FrameData)?
            {
                return Err(RustApngErrorKind::FrameData);
            }
            let mut rgba = Vec::with_capacity(pixel_count * 4);
            for pixel in output.chunks_exact(2) {
                rgba.extend_from_slice(&[pixel[0], pixel[0], pixel[0], pixel[1]]);
            }
            Ok(rgba)
        }
        ColorType::Indexed => Err(RustApngErrorKind::FrameData),
    }
}

fn frame_png_data(parsed: &ParsedApng, frame: &RawFrame) -> Result<Vec<u8>, RustApngErrorKind> {
    let mut png = Vec::with_capacity(PNG_SIGNATURE.len() + frame.image_data.len() + 128);
    png.extend_from_slice(&PNG_SIGNATURE);

    let mut ihdr = parsed.ihdr_data.clone();
    write_u32(&mut ihdr, 0, frame.control.width)?;
    write_u32(&mut ihdr, 4, frame.control.height)?;
    append_png_chunk(&mut png, b"IHDR", &ihdr)?;
    for chunk in &parsed.header_chunks {
        append_png_chunk(&mut png, &chunk.kind, &chunk.data)?;
    }
    append_png_chunk(&mut png, b"IDAT", &frame.image_data)?;
    append_png_chunk(&mut png, b"IEND", &[])?;
    Ok(png)
}

fn rect_from_frame(frame: &RawFrame) -> Result<Rect, RustApngErrorKind> {
    Ok(Rect {
        x: usize::try_from(frame.control.x_offset).map_err(|_| RustApngErrorKind::Apng)?,
        y: usize::try_from(frame.control.y_offset).map_err(|_| RustApngErrorKind::Apng)?,
        width: usize::try_from(frame.control.width).map_err(|_| RustApngErrorKind::Apng)?,
        height: usize::try_from(frame.control.height).map_err(|_| RustApngErrorKind::Apng)?,
    })
}

fn blend_frame(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
    source: &[u8],
    blend_op: BlendOp,
) -> Result<(), RustApngErrorKind> {
    let region = RgbaRegion::new(rect)?;
    region.validate_data_len(source)?;

    for row in 0..region.height() {
        let dst_row = region.canvas_row_mut(canvas, canvas_width, row)?;
        let src_row = region.data_row(source, row)?;

        match blend_op {
            BlendOp::Source => dst_row.copy_from_slice(src_row),
            BlendOp::Over => {
                for (dst_pixel, src_pixel) in
                    dst_row.chunks_exact_mut(4).zip(src_row.chunks_exact(4))
                {
                    blend_pixel_over(dst_pixel, src_pixel);
                }
            }
        }
    }

    Ok(())
}

fn blend_pixel_over(dst: &mut [u8], src: &[u8]) {
    let inverse_alpha = u16::from(255 - src[3]);
    dst[0] = over_channel(src[0], dst[0], inverse_alpha);
    dst[1] = over_channel(src[1], dst[1], inverse_alpha);
    dst[2] = over_channel(src[2], dst[2], inverse_alpha);
    dst[3] = over_channel(src[3], dst[3], inverse_alpha);
}

fn over_channel(src: u8, dst: u8, inverse_alpha: u16) -> u8 {
    let value = u16::from(src) + div_255_u16(u16::from(dst) * inverse_alpha);
    cmp::min(value, 255) as u8
}

fn premultiply_rgba(rgba: &mut [u8]) {
    for pixel in rgba.chunks_exact_mut(4) {
        let alpha = u16::from(pixel[3]);
        pixel[0] = div_255_u16(u16::from(pixel[0]) * alpha) as u8;
        pixel[1] = div_255_u16(u16::from(pixel[1]) * alpha) as u8;
        pixel[2] = div_255_u16(u16::from(pixel[2]) * alpha) as u8;
    }
}

fn div_255_u16(value: u16) -> u16 {
    (value + 127) / 255
}

fn copy_region(
    canvas: &[u8],
    canvas_width: usize,
    rect: Rect,
) -> Result<Vec<u8>, RustApngErrorKind> {
    let region = RgbaRegion::new(rect)?;
    let mut data = Vec::with_capacity(region.byte_len());
    for row in 0..region.height() {
        data.extend_from_slice(region.canvas_row(canvas, canvas_width, row)?);
    }
    Ok(data)
}

fn restore_region(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
    region: &[u8],
) -> Result<(), RustApngErrorKind> {
    let area = RgbaRegion::new(rect)?;
    area.validate_data_len(region)?;

    for row in 0..area.height() {
        let dst_row = area.canvas_row_mut(canvas, canvas_width, row)?;
        let src_row = area.data_row(region, row)?;
        dst_row.copy_from_slice(src_row);
    }
    Ok(())
}

fn clear_region(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
) -> Result<(), RustApngErrorKind> {
    let region = RgbaRegion::new(rect)?;
    for row in 0..region.height() {
        region.canvas_row_mut(canvas, canvas_width, row)?.fill(0);
    }
    Ok(())
}

fn canvas_offset(canvas_width: usize, x: usize, y: usize) -> Result<usize, RustApngErrorKind> {
    y.checked_mul(canvas_width)
        .and_then(|offset| offset.checked_add(x))
        .and_then(|offset| offset.checked_mul(RGBA_BYTES_PER_PIXEL))
        .ok_or(RustApngErrorKind::Apng)
}

fn frame_delay(control: &FrameControl) -> i32 {
    let denominator = if control.delay_den == 0 {
        APNG_DEFAULT_DELAY_DENOMINATOR
    } else {
        control.delay_den
    };
    let delay = u64::from(control.delay_num) * 1000 / u64::from(denominator);
    cmp::min(delay, i32::MAX as u64) as i32
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
        .and_then(|pixels| pixels.checked_mul(4))
        .ok_or(error_kind)?;
    if len > isize::MAX as u64 {
        return Err(error_kind);
    }
    usize::try_from(len).map_err(|_| error_kind)
}

fn append_png_chunk(
    png: &mut Vec<u8>,
    kind: &[u8; 4],
    payload: &[u8],
) -> Result<(), RustApngErrorKind> {
    let length = u32::try_from(payload.len()).map_err(|_| RustApngErrorKind::Apng)?;
    png.extend_from_slice(&length.to_be_bytes());
    png.extend_from_slice(kind);
    png.extend_from_slice(payload);
    png.extend_from_slice(&crc32(kind, payload).to_be_bytes());
    Ok(())
}

fn read_png_chunk(data: &[u8], offset: usize) -> Result<PngChunkView<'_>, RustApngErrorKind> {
    if data.len().saturating_sub(offset) < 12 {
        return Err(RustApngErrorKind::Png);
    }

    let length = read_u32(data, offset).ok_or(RustApngErrorKind::Png)?;
    let length = usize::try_from(length).map_err(|_| RustApngErrorKind::Png)?;
    let chunk_type_offset = offset.checked_add(4).ok_or(RustApngErrorKind::Png)?;
    let chunk_data_offset = offset.checked_add(8).ok_or(RustApngErrorKind::Png)?;
    let chunk_data_end = chunk_data_offset
        .checked_add(length)
        .ok_or(RustApngErrorKind::Png)?;
    let chunk_end = chunk_data_end
        .checked_add(4)
        .ok_or(RustApngErrorKind::Png)?;
    if chunk_end > data.len() {
        return Err(RustApngErrorKind::Png);
    }

    let kind = data[chunk_type_offset..chunk_type_offset + 4]
        .try_into()
        .map_err(|_| RustApngErrorKind::Png)?;
    let chunk_data = &data[chunk_data_offset..chunk_data_end];
    let stored_crc = read_u32(data, chunk_data_end).ok_or(RustApngErrorKind::Png)?;
    if stored_crc != crc32(&kind, chunk_data) {
        return Err(chunk_error_kind(kind));
    }

    Ok(PngChunkView {
        kind,
        data: chunk_data,
        next_offset: chunk_end,
    })
}

fn chunk_error_kind(kind: [u8; 4]) -> RustApngErrorKind {
    match &kind {
        b"acTL" | b"fcTL" | b"fdAT" => RustApngErrorKind::Apng,
        _ => RustApngErrorKind::Png,
    }
}

fn read_u16(data: &[u8], offset: usize) -> Option<u16> {
    Some(u16::from_be_bytes(
        data.get(offset..offset.checked_add(2)?)?.try_into().ok()?,
    ))
}

fn read_u32(data: &[u8], offset: usize) -> Option<u32> {
    Some(u32::from_be_bytes(
        data.get(offset..offset.checked_add(4)?)?.try_into().ok()?,
    ))
}

fn write_u32(data: &mut [u8], offset: usize, value: u32) -> Result<(), RustApngErrorKind> {
    let target = data
        .get_mut(offset..offset.saturating_add(4))
        .ok_or(RustApngErrorKind::Apng)?;
    target.copy_from_slice(&value.to_be_bytes());
    Ok(())
}

fn crc32(kind: &[u8; 4], payload: &[u8]) -> u32 {
    let mut crc = 0xffff_ffff_u32;
    for byte in kind.iter().chain(payload.iter()) {
        crc ^= u32::from(*byte);
        for _ in 0..8 {
            let mask = 0_u32.wrapping_sub(crc & 1);
            crc = (crc >> 1) ^ (0xedb8_8320 & mask);
        }
    }
    crc ^ 0xffff_ffff_u32
}

#[cfg(test)]
mod tests {
    use super::*;

    struct FrameSpec {
        width: u32,
        height: u32,
        x_offset: u32,
        y_offset: u32,
        delay_num: u16,
        delay_den: u16,
        dispose_op: DisposeOp,
        blend_op: BlendOp,
        pixels: Vec<u8>,
    }

    impl FrameSpec {
        fn full_canvas(width: u32, height: u32, pixels: Vec<u8>) -> Self {
            Self {
                width,
                height,
                x_offset: 0,
                y_offset: 0,
                delay_num: 1,
                delay_den: 10,
                dispose_op: DisposeOp::None,
                blend_op: BlendOp::Source,
                pixels,
            }
        }
    }

    fn encode_rgba_png(width: u32, height: u32, pixels: &[u8]) -> Vec<u8> {
        let mut png_data = Vec::new();
        let mut encoder = png::Encoder::new(&mut png_data, width, height);
        encoder.set_color(ColorType::Rgba);
        encoder.set_depth(BitDepth::Eight);
        {
            let mut writer = encoder.write_header().expect("png header should encode");
            writer
                .write_image_data(pixels)
                .expect("png image should encode");
        }
        png_data
    }

    fn append_test_chunk(png: &mut Vec<u8>, kind: &[u8; 4], payload: &[u8]) {
        assert!(append_png_chunk(png, kind, payload).is_ok());
    }

    fn extract_chunks(data: &[u8], expected_kind: &[u8; 4]) -> Vec<Vec<u8>> {
        let mut offset = PNG_SIGNATURE.len();
        let mut chunks = Vec::new();
        while offset < data.len() {
            let chunk = match read_png_chunk(data, offset) {
                Ok(chunk) => chunk,
                Err(_) => panic!("chunk should parse"),
            };
            if &chunk.kind == expected_kind {
                chunks.push(chunk.data.to_vec());
            }
            offset = chunk.next_offset;
        }
        chunks
    }

    fn extract_ihdr(data: &[u8]) -> Vec<u8> {
        extract_chunks(data, b"IHDR")
            .into_iter()
            .next()
            .expect("png should have ihdr")
    }

    fn extract_image_data(data: &[u8]) -> Vec<u8> {
        extract_chunks(data, b"IDAT").concat()
    }

    fn chunk_offset(data: &[u8], expected_kind: &[u8; 4]) -> usize {
        let mut offset = PNG_SIGNATURE.len();
        while offset < data.len() {
            let chunk = match read_png_chunk(data, offset) {
                Ok(chunk) => chunk,
                Err(_) => panic!("chunk should parse"),
            };
            if &chunk.kind == expected_kind {
                return offset;
            }
            offset = chunk.next_offset;
        }
        panic!("expected chunk should exist");
    }

    fn corrupt_chunk_crc(data: &mut [u8], expected_kind: &[u8; 4]) {
        let offset = chunk_offset(data, expected_kind);
        let length = read_u32(data, offset).expect("chunk length") as usize;
        data[offset + 8 + length + 3] ^= 0x01;
    }

    fn frame_control_payload(sequence_number: u32, frame: &FrameSpec) -> Vec<u8> {
        let mut payload = Vec::with_capacity(26);
        payload.extend_from_slice(&sequence_number.to_be_bytes());
        payload.extend_from_slice(&frame.width.to_be_bytes());
        payload.extend_from_slice(&frame.height.to_be_bytes());
        payload.extend_from_slice(&frame.x_offset.to_be_bytes());
        payload.extend_from_slice(&frame.y_offset.to_be_bytes());
        payload.extend_from_slice(&frame.delay_num.to_be_bytes());
        payload.extend_from_slice(&frame.delay_den.to_be_bytes());
        payload.push(match frame.dispose_op {
            DisposeOp::None => 0,
            DisposeOp::Background => 1,
            DisposeOp::Previous => 2,
        });
        payload.push(match frame.blend_op {
            BlendOp::Source => 0,
            BlendOp::Over => 1,
        });
        payload
    }

    fn make_apng(
        canvas_width: u32,
        canvas_height: u32,
        play_count: u32,
        frames: &[FrameSpec],
    ) -> Vec<u8> {
        let first_png = encode_rgba_png(canvas_width, canvas_height, &frames[0].pixels);
        let mut apng = Vec::new();
        apng.extend_from_slice(&PNG_SIGNATURE);
        append_test_chunk(&mut apng, b"IHDR", &extract_ihdr(&first_png));
        let mut animation_control = Vec::new();
        animation_control.extend_from_slice(&(frames.len() as u32).to_be_bytes());
        animation_control.extend_from_slice(&play_count.to_be_bytes());
        append_test_chunk(&mut apng, b"acTL", &animation_control);

        let mut sequence_number = 0;
        append_test_chunk(
            &mut apng,
            b"fcTL",
            &frame_control_payload(sequence_number, &frames[0]),
        );
        sequence_number += 1;
        append_test_chunk(&mut apng, b"IDAT", &extract_image_data(&first_png));

        for frame in &frames[1..] {
            append_test_chunk(
                &mut apng,
                b"fcTL",
                &frame_control_payload(sequence_number, frame),
            );
            sequence_number += 1;
            let frame_png = encode_rgba_png(frame.width, frame.height, &frame.pixels);
            let mut frame_data = Vec::new();
            frame_data.extend_from_slice(&sequence_number.to_be_bytes());
            frame_data.extend_from_slice(&extract_image_data(&frame_png));
            append_test_chunk(&mut apng, b"fdAT", &frame_data);
            sequence_number += 1;
        }

        append_test_chunk(&mut apng, b"IEND", &[]);
        apng
    }

    fn decoded_animation(data: &[u8]) -> DecodedApngAnimation {
        match decode_apng(data) {
            DecodeOutcome::Success(animation) => animation,
            DecodeOutcome::NotApng => panic!("expected APNG, got NotApng"),
            DecodeOutcome::Error(_) => panic!("expected APNG, got error"),
        }
    }

    fn pixel(frame: &DecodedApngFrame, index: usize) -> [u8; 4] {
        frame.rgba[index * 4..index * 4 + 4]
            .try_into()
            .expect("pixel should be present")
    }

    fn premul(pixel: [u8; 4]) -> [u8; 4] {
        let mut rgba = pixel.to_vec();
        premultiply_rgba(&mut rgba);
        rgba.try_into().expect("premul pixel")
    }

    #[test]
    fn non_png_and_plain_png_return_not_apng() {
        assert!(matches!(decode_apng(b"not png"), DecodeOutcome::NotApng));

        let png = encode_rgba_png(1, 1, &[255, 0, 0, 255]);
        assert!(matches!(decode_apng(&png), DecodeOutcome::NotApng));
    }

    #[test]
    fn malformed_png_and_apng_chunks_return_error() {
        assert!(matches!(
            decode_apng(&PNG_SIGNATURE),
            DecodeOutcome::Error(RustApngErrorKind::Png)
        ));

        let base_png = encode_rgba_png(1, 1, &[0, 0, 0, 0]);
        let mut apng = Vec::new();
        apng.extend_from_slice(&PNG_SIGNATURE);
        append_test_chunk(&mut apng, b"IHDR", &extract_ihdr(&base_png));
        append_test_chunk(&mut apng, b"acTL", &[0, 0, 0]);
        append_test_chunk(&mut apng, b"IEND", &[]);

        assert!(matches!(
            decode_apng(&apng),
            DecodeOutcome::Error(RustApngErrorKind::Apng)
        ));
    }

    #[test]
    fn validates_animation_chunk_ordering() {
        let base_png = encode_rgba_png(1, 1, &[0, 0, 0, 0]);
        let mut actl_after_idat = Vec::new();
        actl_after_idat.extend_from_slice(&PNG_SIGNATURE);
        append_test_chunk(&mut actl_after_idat, b"IHDR", &extract_ihdr(&base_png));
        append_test_chunk(
            &mut actl_after_idat,
            b"IDAT",
            &extract_image_data(&base_png),
        );
        append_test_chunk(&mut actl_after_idat, b"acTL", &[0, 0, 0, 1, 0, 0, 0, 0]);
        append_test_chunk(&mut actl_after_idat, b"IEND", &[]);
        assert!(matches!(
            decode_apng(&actl_after_idat),
            DecodeOutcome::Error(RustApngErrorKind::Apng)
        ));

        let frame = FrameSpec::full_canvas(1, 1, vec![0, 0, 0, 0]);
        let mut fdat_before_idat = Vec::new();
        fdat_before_idat.extend_from_slice(&PNG_SIGNATURE);
        append_test_chunk(&mut fdat_before_idat, b"IHDR", &extract_ihdr(&base_png));
        append_test_chunk(&mut fdat_before_idat, b"acTL", &[0, 0, 0, 1, 0, 0, 0, 0]);
        append_test_chunk(
            &mut fdat_before_idat,
            b"fcTL",
            &frame_control_payload(0, &frame),
        );
        append_test_chunk(&mut fdat_before_idat, b"fdAT", &[1, 0, 0, 0]);
        append_test_chunk(&mut fdat_before_idat, b"IEND", &[]);
        assert!(matches!(
            decode_apng(&fdat_before_idat),
            DecodeOutcome::Error(RustApngErrorKind::Apng)
        ));

        let mut fctl_before_actl = Vec::new();
        fctl_before_actl.extend_from_slice(&PNG_SIGNATURE);
        append_test_chunk(&mut fctl_before_actl, b"IHDR", &extract_ihdr(&base_png));
        append_test_chunk(
            &mut fctl_before_actl,
            b"fcTL",
            &frame_control_payload(0, &frame),
        );
        append_test_chunk(&mut fctl_before_actl, b"IEND", &[]);
        assert!(matches!(
            decode_apng(&fctl_before_actl),
            DecodeOutcome::Error(RustApngErrorKind::Apng)
        ));
    }

    #[test]
    fn validates_animation_sequence_numbers() {
        let base_png = encode_rgba_png(1, 1, &[0, 0, 0, 0]);
        let frame = FrameSpec::full_canvas(1, 1, vec![0, 0, 0, 0]);
        let mut wrong_fctl_sequence = Vec::new();
        wrong_fctl_sequence.extend_from_slice(&PNG_SIGNATURE);
        append_test_chunk(&mut wrong_fctl_sequence, b"IHDR", &extract_ihdr(&base_png));
        append_test_chunk(&mut wrong_fctl_sequence, b"acTL", &[0, 0, 0, 1, 0, 0, 0, 0]);
        append_test_chunk(
            &mut wrong_fctl_sequence,
            b"fcTL",
            &frame_control_payload(1, &frame),
        );
        append_test_chunk(
            &mut wrong_fctl_sequence,
            b"IDAT",
            &extract_image_data(&base_png),
        );
        append_test_chunk(&mut wrong_fctl_sequence, b"IEND", &[]);
        assert!(matches!(
            decode_apng(&wrong_fctl_sequence),
            DecodeOutcome::Error(RustApngErrorKind::Apng)
        ));

        let first = FrameSpec::full_canvas(1, 1, vec![0, 0, 0, 0]);
        let second = FrameSpec {
            width: 1,
            height: 1,
            x_offset: 0,
            y_offset: 0,
            delay_num: 1,
            delay_den: 10,
            dispose_op: DisposeOp::None,
            blend_op: BlendOp::Source,
            pixels: vec![255, 0, 0, 255],
        };
        let mut wrong_fdat_sequence = make_apng(1, 1, 1, &[first, second]);
        let fdat_offset = chunk_offset(&wrong_fdat_sequence, b"fdAT");
        wrong_fdat_sequence[fdat_offset + 8..fdat_offset + 12]
            .copy_from_slice(&99_u32.to_be_bytes());
        let length = read_u32(&wrong_fdat_sequence, fdat_offset).expect("fdAT length") as usize;
        let crc = crc32(
            b"fdAT",
            &wrong_fdat_sequence[fdat_offset + 8..fdat_offset + 8 + length],
        );
        wrong_fdat_sequence[fdat_offset + 8 + length..fdat_offset + 12 + length]
            .copy_from_slice(&crc.to_be_bytes());

        assert!(matches!(
            decode_apng(&wrong_fdat_sequence),
            DecodeOutcome::Error(RustApngErrorKind::Apng)
        ));
    }

    #[test]
    fn crc_mismatch_returns_parse_error() {
        let first = FrameSpec::full_canvas(1, 1, vec![0, 0, 0, 0]);
        let mut apng = make_apng(1, 1, 1, &[first]);
        corrupt_chunk_crc(&mut apng, b"acTL");

        assert!(matches!(
            decode_apng(&apng),
            DecodeOutcome::Error(RustApngErrorKind::Apng)
        ));
    }

    #[test]
    fn delay_denominator_zero_and_loop_count_zero_are_normalized() {
        let mut frame = FrameSpec::full_canvas(1, 1, vec![255, 0, 0, 255]);
        frame.delay_num = 5;
        frame.delay_den = 0;
        let apng = make_apng(1, 1, 0, &[frame]);

        let animation = decoded_animation(&apng);

        assert_eq!(animation.loop_count, -1);
        assert_eq!(animation.frames[0].delay, 50);
    }

    #[test]
    fn blend_source_replaces_existing_pixels() {
        let first = FrameSpec::full_canvas(1, 1, vec![255, 0, 0, 255]);
        let second = FrameSpec {
            width: 1,
            height: 1,
            x_offset: 0,
            y_offset: 0,
            delay_num: 1,
            delay_den: 10,
            dispose_op: DisposeOp::None,
            blend_op: BlendOp::Source,
            pixels: vec![0, 0, 255, 0],
        };
        let apng = make_apng(1, 1, 1, &[first, second]);

        let animation = decoded_animation(&apng);

        assert_eq!(pixel(&animation.frames[1], 0), [0, 0, 0, 0]);
    }

    #[test]
    fn blend_over_composites_premultiplied_pixels() {
        let first = FrameSpec::full_canvas(1, 1, vec![255, 0, 0, 255]);
        let second = FrameSpec {
            width: 1,
            height: 1,
            x_offset: 0,
            y_offset: 0,
            delay_num: 1,
            delay_den: 10,
            dispose_op: DisposeOp::None,
            blend_op: BlendOp::Over,
            pixels: vec![0, 0, 255, 128],
        };
        let apng = make_apng(1, 1, 1, &[first, second]);

        let animation = decoded_animation(&apng);

        assert_eq!(pixel(&animation.frames[1], 0), [127, 0, 128, 255]);
    }

    #[test]
    fn region_operations_report_out_of_bounds_canvas() {
        let rect = Rect {
            x: 1,
            y: 0,
            width: 1,
            height: 1,
        };
        let source = vec![255, 0, 0, 255];
        let mut canvas = vec![0; 4];

        assert!(blend_frame(&mut canvas, 1, rect, &source, BlendOp::Source).is_err());
        assert!(copy_region(&canvas, 1, rect).is_err());
        assert!(clear_region(&mut canvas, 1, rect).is_err());
        assert!(restore_region(&mut canvas, 1, rect, &source).is_err());
    }

    #[test]
    fn region_operations_reject_mismatched_source_lengths() {
        let rect = Rect {
            x: 0,
            y: 0,
            width: 1,
            height: 1,
        };
        let mut canvas = vec![0; 4];
        let too_short = vec![255, 0, 0];

        assert!(blend_frame(&mut canvas, 1, rect, &too_short, BlendOp::Source).is_err());
        assert!(restore_region(&mut canvas, 1, rect, &too_short).is_err());
    }

    #[test]
    fn dispose_background_clears_after_frame() {
        let mut first = FrameSpec::full_canvas(2, 1, vec![255, 0, 0, 255, 0, 0, 0, 0]);
        first.dispose_op = DisposeOp::Background;
        let second = FrameSpec {
            width: 1,
            height: 1,
            x_offset: 1,
            y_offset: 0,
            delay_num: 1,
            delay_den: 10,
            dispose_op: DisposeOp::None,
            blend_op: BlendOp::Source,
            pixels: vec![0, 0, 255, 255],
        };
        let apng = make_apng(2, 1, 1, &[first, second]);

        let animation = decoded_animation(&apng);

        assert_eq!(pixel(&animation.frames[1], 0), [0, 0, 0, 0]);
        assert_eq!(pixel(&animation.frames[1], 1), premul([0, 0, 255, 255]));
    }

    #[test]
    fn dispose_previous_restores_prior_canvas() {
        let first = FrameSpec::full_canvas(2, 1, vec![255, 0, 0, 255, 0, 0, 0, 0]);
        let second = FrameSpec {
            width: 1,
            height: 1,
            x_offset: 0,
            y_offset: 0,
            delay_num: 1,
            delay_den: 10,
            dispose_op: DisposeOp::Previous,
            blend_op: BlendOp::Source,
            pixels: vec![0, 0, 255, 255],
        };
        let third = FrameSpec {
            width: 1,
            height: 1,
            x_offset: 1,
            y_offset: 0,
            delay_num: 1,
            delay_den: 10,
            dispose_op: DisposeOp::None,
            blend_op: BlendOp::Source,
            pixels: vec![0, 255, 0, 255],
        };
        let apng = make_apng(2, 1, 1, &[first, second, third]);

        let animation = decoded_animation(&apng);

        assert_eq!(pixel(&animation.frames[2], 0), premul([255, 0, 0, 255]));
        assert_eq!(pixel(&animation.frames[2], 1), premul([0, 255, 0, 255]));
    }

    #[test]
    fn tiny_generated_apng_decodes_expected_frames_and_pixels() {
        let first = FrameSpec::full_canvas(2, 1, vec![255, 0, 0, 255, 0, 0, 0, 0]);
        let mut second = FrameSpec {
            width: 1,
            height: 1,
            x_offset: 1,
            y_offset: 0,
            delay_num: 2,
            delay_den: 10,
            dispose_op: DisposeOp::None,
            blend_op: BlendOp::Source,
            pixels: vec![0, 0, 255, 255],
        };
        second.delay_num = 2;
        let apng = make_apng(2, 1, 2, &[first, second]);

        let animation = decoded_animation(&apng);

        assert_eq!(animation.width, 2);
        assert_eq!(animation.height, 1);
        assert_eq!(animation.loop_count, 1);
        assert_eq!(animation.frames.len(), 2);
        assert_eq!(animation.frames[0].delay, 100);
        assert_eq!(animation.frames[1].delay, 200);
        assert_eq!(pixel(&animation.frames[0], 0), premul([255, 0, 0, 255]));
        assert_eq!(pixel(&animation.frames[0], 1), [0, 0, 0, 0]);
        assert_eq!(pixel(&animation.frames[1], 0), premul([255, 0, 0, 255]));
        assert_eq!(pixel(&animation.frames[1], 1), premul([0, 0, 255, 255]));
    }
}
