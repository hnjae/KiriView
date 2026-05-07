// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::byteio::{read_be_u16, read_be_u32};

use super::{RustApngErrorKind, rgba_len};

pub(super) const PNG_SIGNATURE: [u8; 8] = [0x89, b'P', b'N', b'G', 0x0d, 0x0a, 0x1a, 0x0a];

const MAX_QIMAGE_DIMENSION: u32 = i32::MAX as u32;

#[derive(Clone)]
pub(super) struct PngChunk {
    pub(super) kind: [u8; 4],
    pub(super) data: Vec<u8>,
}

pub(super) struct PngChunkView<'a> {
    pub(super) kind: [u8; 4],
    pub(super) data: &'a [u8],
    pub(super) next_offset: usize,
}

#[derive(Clone, Copy)]
pub(super) struct FrameControl {
    pub(super) sequence_number: u32,
    pub(super) width: u32,
    pub(super) height: u32,
    pub(super) x_offset: u32,
    pub(super) y_offset: u32,
    pub(super) delay_num: u16,
    pub(super) delay_den: u16,
    pub(super) dispose_op: DisposeOp,
    pub(super) blend_op: BlendOp,
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub(super) enum DisposeOp {
    None,
    Background,
    Previous,
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub(super) enum BlendOp {
    Source,
    Over,
}

pub(super) struct RawFrame {
    pub(super) control: FrameControl,
    pub(super) image_data: Vec<u8>,
    uses_default_image_data: bool,
}

pub(super) struct ParsedApng {
    pub(super) canvas_width: u32,
    pub(super) canvas_height: u32,
    pub(super) play_count: u32,
    pub(super) ihdr_data: Vec<u8>,
    pub(super) header_chunks: Vec<PngChunk>,
    pub(super) frames: Vec<RawFrame>,
}

#[derive(Clone, Copy, Eq, PartialEq)]
enum PngChunkPhase {
    BeforeIhdr,
    BeforeIdat,
    ReadingIdat,
    AfterIdat,
    CompleteBeforeIdat,
    CompleteAfterIdat,
}

impl PngChunkPhase {
    fn has_ihdr(self) -> bool {
        self != Self::BeforeIhdr
    }

    fn has_idat(self) -> bool {
        matches!(
            self,
            Self::ReadingIdat | Self::AfterIdat | Self::CompleteAfterIdat
        )
    }

    fn idat_chunks_open(self) -> bool {
        self == Self::ReadingIdat
    }

    fn complete(self) -> Self {
        if self.has_idat() {
            Self::CompleteAfterIdat
        } else {
            Self::CompleteBeforeIdat
        }
    }

    fn is_complete(self) -> bool {
        matches!(self, Self::CompleteBeforeIdat | Self::CompleteAfterIdat)
    }
}

struct AnimationControl {
    expected_frame_count: u32,
}

struct ApngParseState {
    phase: PngChunkPhase,
    animation_control: Option<AnimationControl>,
    expected_sequence_number: u32,
    parsed: ParsedApng,
    current_frame: Option<RawFrame>,
}

impl ApngParseState {
    fn new() -> Self {
        Self {
            phase: PngChunkPhase::BeforeIhdr,
            animation_control: None,
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
        self.phase.is_complete()
    }

    fn handle_chunk(&mut self, chunk: PngChunkView<'_>) -> Result<(), RustApngErrorKind> {
        let kind = chunk.kind;
        let chunk_data = chunk.data;

        if !self.phase.has_ihdr() && kind != *b"IHDR" {
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
        if !self.phase.has_ihdr() || !self.phase.is_complete() {
            return Err(RustApngErrorKind::Png);
        }
        let Some(animation_control) = self.animation_control else {
            return Ok(None);
        };
        if !self.phase.has_idat()
            || usize::try_from(animation_control.expected_frame_count).ok()
                != Some(self.parsed.frames.len())
        {
            return Err(RustApngErrorKind::Apng);
        }

        Ok(Some(self.parsed))
    }

    fn handle_ihdr(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if self.phase.has_ihdr() || !self.parsed.ihdr_data.is_empty() || chunk_data.len() != 13 {
            return Err(RustApngErrorKind::Png);
        }
        self.phase = PngChunkPhase::BeforeIdat;
        self.parsed.ihdr_data = chunk_data.to_vec();
        self.parsed.canvas_width = read_be_u32(chunk_data, 0).ok_or(RustApngErrorKind::Png)?;
        self.parsed.canvas_height = read_be_u32(chunk_data, 4).ok_or(RustApngErrorKind::Png)?;
        validate_dimensions(
            self.parsed.canvas_width,
            self.parsed.canvas_height,
            RustApngErrorKind::Png,
        )
    }

    fn handle_actl(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if !self.phase.has_ihdr()
            || self.animation_control.is_some()
            || self.phase.has_idat()
            || chunk_data.len() != 8
        {
            return Err(RustApngErrorKind::Apng);
        }
        let expected_frame_count = read_be_u32(chunk_data, 0).ok_or(RustApngErrorKind::Apng)?;
        self.parsed.play_count = read_be_u32(chunk_data, 4).ok_or(RustApngErrorKind::Apng)?;
        if expected_frame_count == 0 {
            return Err(RustApngErrorKind::Apng);
        }
        self.animation_control = Some(AnimationControl {
            expected_frame_count,
        });
        Ok(())
    }

    fn handle_fctl(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if self.animation_control.is_none() {
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
            uses_default_image_data: !self.phase.has_idat(),
        });
        self.close_idat_chunks();
        Ok(())
    }

    fn handle_idat(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if !self.phase.has_ihdr() {
            return Err(RustApngErrorKind::Png);
        }
        if self.phase.has_idat() && !self.phase.idat_chunks_open() {
            return Err(RustApngErrorKind::Png);
        }
        if self
            .current_frame
            .as_ref()
            .is_some_and(|frame| !frame.uses_default_image_data)
        {
            return Err(RustApngErrorKind::Apng);
        }

        self.phase = PngChunkPhase::ReadingIdat;
        if let Some(frame) = self.current_frame.as_mut()
            && frame.uses_default_image_data
        {
            frame.image_data.extend_from_slice(chunk_data);
        }
        Ok(())
    }

    fn handle_fdat(&mut self, chunk_data: &[u8]) -> Result<(), RustApngErrorKind> {
        if self.animation_control.is_none() || !self.phase.has_idat() || chunk_data.len() < 4 {
            return Err(RustApngErrorKind::Apng);
        }
        let sequence_number = read_be_u32(chunk_data, 0).ok_or(RustApngErrorKind::Apng)?;
        if sequence_number != self.expected_sequence_number {
            return Err(RustApngErrorKind::Apng);
        }
        self.advance_sequence_number()?;
        let frame = self.current_frame.as_mut().ok_or(RustApngErrorKind::Apng)?;
        if frame.uses_default_image_data {
            return Err(RustApngErrorKind::Apng);
        }
        frame.image_data.extend_from_slice(&chunk_data[4..]);
        self.close_idat_chunks();
        Ok(())
    }

    fn handle_iend(&mut self) -> Result<(), RustApngErrorKind> {
        self.finish_current_frame()?;
        self.phase = self.phase.complete();
        Ok(())
    }

    fn handle_other_chunk(&mut self, kind: [u8; 4], chunk_data: &[u8]) {
        if !self.phase.has_idat() {
            self.parsed.header_chunks.push(PngChunk {
                kind,
                data: chunk_data.to_vec(),
            });
        } else {
            self.close_idat_chunks();
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

    fn close_idat_chunks(&mut self) {
        if self.phase == PngChunkPhase::ReadingIdat {
            self.phase = PngChunkPhase::AfterIdat;
        }
    }
}

pub(super) fn parse_apng(data: &[u8]) -> Result<Option<ParsedApng>, RustApngErrorKind> {
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
        sequence_number: read_be_u32(data, 0).ok_or(RustApngErrorKind::Apng)?,
        width: read_be_u32(data, 4).ok_or(RustApngErrorKind::Apng)?,
        height: read_be_u32(data, 8).ok_or(RustApngErrorKind::Apng)?,
        x_offset: read_be_u32(data, 12).ok_or(RustApngErrorKind::Apng)?,
        y_offset: read_be_u32(data, 16).ok_or(RustApngErrorKind::Apng)?,
        delay_num: read_be_u16(data, 20).ok_or(RustApngErrorKind::Apng)?,
        delay_den: read_be_u16(data, 22).ok_or(RustApngErrorKind::Apng)?,
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

pub(super) fn append_png_chunk(
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

pub(super) fn read_png_chunk(
    data: &[u8],
    offset: usize,
) -> Result<PngChunkView<'_>, RustApngErrorKind> {
    if data.len().saturating_sub(offset) < 12 {
        return Err(RustApngErrorKind::Png);
    }

    let length = read_be_u32(data, offset).ok_or(RustApngErrorKind::Png)?;
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
    let stored_crc = read_be_u32(data, chunk_data_end).ok_or(RustApngErrorKind::Png)?;
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

pub(super) fn crc32(kind: &[u8; 4], payload: &[u8]) -> u32 {
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
