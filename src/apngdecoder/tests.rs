// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use super::parser::{BlendOp, DisposeOp, PNG_SIGNATURE, append_png_chunk, crc32, read_png_chunk};
use super::render::{
    Rect, blend_frame, clear_region, copy_region, premultiply_rgba, restore_region,
};
use super::*;
use crate::byteio::read_be_u32;
use png::{BitDepth, ColorType};

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
    let length = read_be_u32(data, offset).expect("chunk length") as usize;
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
    wrong_fdat_sequence[fdat_offset + 8..fdat_offset + 12].copy_from_slice(&99_u32.to_be_bytes());
    let length = read_be_u32(&wrong_fdat_sequence, fdat_offset).expect("fdAT length") as usize;
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
