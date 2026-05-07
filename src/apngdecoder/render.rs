// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::byteio::write_be_u32;

use super::parser::{
    BlendOp, DisposeOp, FrameControl, PNG_SIGNATURE, ParsedApng, RawFrame, append_png_chunk,
};
use super::{DecodedApngFrame, RGBA_BYTES_PER_PIXEL, RustApngErrorKind, rgba_len};
use png::{BitDepth, ColorType};
use std::cmp;
use std::io::Cursor;
use std::ops::Range;

const APNG_DEFAULT_DELAY_DENOMINATOR: u16 = 100;

#[derive(Clone, Copy)]
pub(super) struct Rect {
    pub(super) x: usize,
    pub(super) y: usize,
    pub(super) width: usize,
    pub(super) height: usize,
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

    fn canvas_row(
        self,
        canvas: &[u8],
        canvas_width: usize,
        row: usize,
    ) -> Result<&[u8], RustApngErrorKind> {
        canvas
            .get(self.canvas_row_range(canvas_width, row)?)
            .ok_or(RustApngErrorKind::Apng)
    }

    fn canvas_row_mut(
        self,
        canvas: &mut [u8],
        canvas_width: usize,
        row: usize,
    ) -> Result<&mut [u8], RustApngErrorKind> {
        canvas
            .get_mut(self.canvas_row_range(canvas_width, row)?)
            .ok_or(RustApngErrorKind::Apng)
    }

    fn data_row(self, data: &[u8], row: usize) -> Result<&[u8], RustApngErrorKind> {
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

fn visit_canvas_rows<F>(
    canvas: &[u8],
    canvas_width: usize,
    rect: Rect,
    mut visit: F,
) -> Result<(), RustApngErrorKind>
where
    F: FnMut(&[u8]),
{
    let region = RgbaRegion::new(rect)?;
    for row in 0..region.height() {
        visit(region.canvas_row(canvas, canvas_width, row)?);
    }
    Ok(())
}

fn visit_canvas_rows_mut<F>(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
    mut visit: F,
) -> Result<(), RustApngErrorKind>
where
    F: FnMut(&mut [u8]),
{
    let region = RgbaRegion::new(rect)?;
    for row in 0..region.height() {
        visit(region.canvas_row_mut(canvas, canvas_width, row)?);
    }
    Ok(())
}

fn visit_canvas_data_rows_mut<F>(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
    data: &[u8],
    mut visit: F,
) -> Result<(), RustApngErrorKind>
where
    F: FnMut(&mut [u8], &[u8]),
{
    let region = RgbaRegion::new(rect)?;
    region.validate_data_len(data)?;

    for row in 0..region.height() {
        let canvas_row = region.canvas_row_mut(canvas, canvas_width, row)?;
        let data_row = region.data_row(data, row)?;
        visit(canvas_row, data_row);
    }
    Ok(())
}

pub(super) fn render_frames(
    parsed: &ParsedApng,
) -> Result<Vec<DecodedApngFrame>, RustApngErrorKind> {
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
            validate_png_output_len(output, pixel_count, RGBA_BYTES_PER_PIXEL)?;
            Ok(output.to_vec())
        }
        ColorType::Rgb => {
            validate_png_output_len(output, pixel_count, 3)?;
            let mut rgba = Vec::with_capacity(pixel_count * RGBA_BYTES_PER_PIXEL);
            for pixel in output.chunks_exact(3) {
                rgba.extend_from_slice(&[pixel[0], pixel[1], pixel[2], 255]);
            }
            Ok(rgba)
        }
        ColorType::Grayscale => {
            validate_png_output_len(output, pixel_count, 1)?;
            let mut rgba = Vec::with_capacity(pixel_count * RGBA_BYTES_PER_PIXEL);
            for gray in output {
                rgba.extend_from_slice(&[*gray, *gray, *gray, 255]);
            }
            Ok(rgba)
        }
        ColorType::GrayscaleAlpha => {
            validate_png_output_len(output, pixel_count, 2)?;
            let mut rgba = Vec::with_capacity(pixel_count * RGBA_BYTES_PER_PIXEL);
            for pixel in output.chunks_exact(2) {
                rgba.extend_from_slice(&[pixel[0], pixel[0], pixel[0], pixel[1]]);
            }
            Ok(rgba)
        }
        ColorType::Indexed => Err(RustApngErrorKind::FrameData),
    }
}

fn validate_png_output_len(
    output: &[u8],
    pixel_count: usize,
    bytes_per_pixel: usize,
) -> Result<(), RustApngErrorKind> {
    let expected_len = pixel_count
        .checked_mul(bytes_per_pixel)
        .ok_or(RustApngErrorKind::FrameData)?;
    if output.len() != expected_len {
        return Err(RustApngErrorKind::FrameData);
    }

    Ok(())
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

pub(super) fn blend_frame(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
    source: &[u8],
    blend_op: BlendOp,
) -> Result<(), RustApngErrorKind> {
    visit_canvas_data_rows_mut(
        canvas,
        canvas_width,
        rect,
        source,
        |dst_row, src_row| match blend_op {
            BlendOp::Source => dst_row.copy_from_slice(src_row),
            BlendOp::Over => {
                for (dst_pixel, src_pixel) in
                    dst_row.chunks_exact_mut(4).zip(src_row.chunks_exact(4))
                {
                    blend_pixel_over(dst_pixel, src_pixel);
                }
            }
        },
    )
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

pub(super) fn premultiply_rgba(rgba: &mut [u8]) {
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

pub(super) fn copy_region(
    canvas: &[u8],
    canvas_width: usize,
    rect: Rect,
) -> Result<Vec<u8>, RustApngErrorKind> {
    let region = RgbaRegion::new(rect)?;
    let mut data = Vec::with_capacity(region.byte_len());
    visit_canvas_rows(canvas, canvas_width, rect, |row| {
        data.extend_from_slice(row)
    })?;
    Ok(data)
}

pub(super) fn restore_region(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
    region: &[u8],
) -> Result<(), RustApngErrorKind> {
    visit_canvas_data_rows_mut(canvas, canvas_width, rect, region, |dst_row, src_row| {
        dst_row.copy_from_slice(src_row);
    })
}

pub(super) fn clear_region(
    canvas: &mut [u8],
    canvas_width: usize,
    rect: Rect,
) -> Result<(), RustApngErrorKind> {
    visit_canvas_rows_mut(canvas, canvas_width, rect, |row| row.fill(0))
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

fn write_u32(data: &mut [u8], offset: usize, value: u32) -> Result<(), RustApngErrorKind> {
    write_be_u32(data, offset, value)
        .then_some(())
        .ok_or(RustApngErrorKind::Apng)
}
