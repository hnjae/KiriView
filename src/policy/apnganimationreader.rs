// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use std::io::{BufReader, Cursor};

use png::{
    BitDepth, BlendOp, ColorType, Decoder, DisposeOp, FrameControl, Reader, Transformations,
};

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustApngOpenStatus {
        NotApng = 0,
        Success = 1,
        Error = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustApngReadStatus {
        Frame = 0,
        End = 1,
        Error = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustApngDisposeOp {
        None = 0,
        Background = 1,
        Previous = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustApngBlendOp {
        Source = 0,
        Over = 1,
    }

    #[derive(Clone, Debug, PartialEq, Eq)]
    struct RustApngOpenResult {
        status: RustApngOpenStatus,
        canvas_width: u32,
        canvas_height: u32,
        loop_count: i32,
        frame_count: i32,
    }

    #[derive(Clone, Debug, PartialEq, Eq)]
    struct RustApngFrameResult {
        status: RustApngReadStatus,
        width: u32,
        height: u32,
        x_offset: u32,
        y_offset: u32,
        delay_num: u32,
        delay_den: u32,
        dispose_op: RustApngDisposeOp,
        blend_op: RustApngBlendOp,
        row_bytes: usize,
        has_more_frames: bool,
        pixels: Vec<u8>,
    }

    extern "Rust" {
        type RustApngAnimationReader;

        #[cxx_name = "rustNewApngAnimationReader"]
        fn rust_new_apng_animation_reader() -> Box<RustApngAnimationReader>;

        #[cxx_name = "rustOpenApngAnimationReader"]
        fn rust_open_apng_animation_reader(
            reader: &mut RustApngAnimationReader,
            data: &[u8],
        ) -> RustApngOpenResult;

        #[cxx_name = "rustReadApngAnimationFrame"]
        fn rust_read_apng_animation_frame(
            reader: &mut RustApngAnimationReader,
        ) -> RustApngFrameResult;

        #[cxx_name = "rustApngAnimationReaderHasMoreFrames"]
        fn rust_apng_animation_reader_has_more_frames(reader: &RustApngAnimationReader) -> bool;
    }
}

use ffi::{
    RustApngBlendOp, RustApngDisposeOp, RustApngFrameResult, RustApngOpenResult,
    RustApngOpenStatus, RustApngReadStatus,
};

const PNG_SIGNATURE: &[u8] = b"\x89PNG\r\n\x1a\n";

type PngReader = Reader<BufReader<Cursor<Vec<u8>>>>;

pub(crate) struct RustApngAnimationReader {
    reader: Option<PngReader>,
    raw_frames_remaining: u32,
}

fn rust_new_apng_animation_reader() -> Box<RustApngAnimationReader> {
    Box::new(RustApngAnimationReader {
        reader: None,
        raw_frames_remaining: 0,
    })
}

fn rust_open_apng_animation_reader(
    state: &mut RustApngAnimationReader,
    data: &[u8],
) -> RustApngOpenResult {
    state.reader = None;
    state.raw_frames_remaining = 0;

    if !data.starts_with(PNG_SIGNATURE) {
        return RustApngOpenResult {
            status: RustApngOpenStatus::NotApng,
            ..open_result_defaults()
        };
    }

    let mut decoder = Decoder::new(BufReader::new(Cursor::new(data.to_vec())));
    decoder.set_transformations(
        Transformations::EXPAND | Transformations::STRIP_16 | Transformations::ALPHA,
    );

    let Ok(reader) = decoder.read_info() else {
        return open_error();
    };

    let info = reader.info();
    let Some(animation) = info.animation_control else {
        return RustApngOpenResult {
            status: RustApngOpenStatus::NotApng,
            ..open_result_defaults()
        };
    };

    if animation.num_frames == 0 {
        return open_error();
    }

    let raw_frames_remaining = animation.num_frames + u32::from(info.frame_control.is_none());
    let frame_count = match i32::try_from(animation.num_frames) {
        Ok(frame_count) if frame_count > 0 => frame_count,
        _ => return open_error(),
    };

    if !is_rgba8_output(&reader) || info.width == 0 || info.height == 0 {
        return open_error();
    }

    let canvas_width = info.width;
    let canvas_height = info.height;
    state.raw_frames_remaining = raw_frames_remaining;
    state.reader = Some(reader);
    RustApngOpenResult {
        status: RustApngOpenStatus::Success,
        canvas_width,
        canvas_height,
        loop_count: apng_loop_count_for_play_count(animation.num_plays),
        frame_count,
    }
}

fn rust_read_apng_animation_frame(state: &mut RustApngAnimationReader) -> RustApngFrameResult {
    let Some(reader) = state.reader.as_mut() else {
        return read_end();
    };

    loop {
        if state.raw_frames_remaining == 0 {
            state.reader = None;
            return read_end();
        }

        let Some(buffer_size) = reader.output_buffer_size() else {
            state.reader = None;
            return read_error();
        };
        let mut pixels = vec![0; buffer_size];
        let output = match reader.next_frame(&mut pixels) {
            Ok(output) => output,
            Err(_) => {
                state.reader = None;
                return read_error();
            }
        };
        state.raw_frames_remaining = state.raw_frames_remaining.saturating_sub(1);

        if !is_rgba8_output(reader) {
            state.reader = None;
            return read_error();
        }

        let Some(control) = reader.info().frame_control else {
            continue;
        };

        pixels.truncate(output.buffer_size());
        return frame_result(
            control,
            output.line_size,
            state.raw_frames_remaining > 0,
            pixels,
        );
    }
}

fn rust_apng_animation_reader_has_more_frames(state: &RustApngAnimationReader) -> bool {
    state.reader.is_some() && state.raw_frames_remaining > 0
}

fn is_rgba8_output(reader: &PngReader) -> bool {
    reader.output_color_type() == (ColorType::Rgba, BitDepth::Eight)
}

fn apng_loop_count_for_play_count(play_count: u32) -> i32 {
    if play_count == 0 {
        -1
    } else {
        i32::try_from(play_count.saturating_sub(1)).unwrap_or(i32::MAX)
    }
}

fn dispose_op(op: DisposeOp) -> RustApngDisposeOp {
    match op {
        DisposeOp::Background => RustApngDisposeOp::Background,
        DisposeOp::Previous => RustApngDisposeOp::Previous,
        DisposeOp::None => RustApngDisposeOp::None,
    }
}

fn blend_op(op: BlendOp) -> RustApngBlendOp {
    match op {
        BlendOp::Over => RustApngBlendOp::Over,
        BlendOp::Source => RustApngBlendOp::Source,
    }
}

fn frame_result(
    control: FrameControl,
    row_bytes: usize,
    has_more_frames: bool,
    pixels: Vec<u8>,
) -> RustApngFrameResult {
    RustApngFrameResult {
        status: RustApngReadStatus::Frame,
        width: control.width,
        height: control.height,
        x_offset: control.x_offset,
        y_offset: control.y_offset,
        delay_num: u32::from(control.delay_num),
        delay_den: u32::from(control.delay_den),
        dispose_op: dispose_op(control.dispose_op),
        blend_op: blend_op(control.blend_op),
        row_bytes,
        has_more_frames,
        pixels,
    }
}

fn open_result_defaults() -> RustApngOpenResult {
    RustApngOpenResult {
        status: RustApngOpenStatus::NotApng,
        canvas_width: 0,
        canvas_height: 0,
        loop_count: 0,
        frame_count: 0,
    }
}

fn open_error() -> RustApngOpenResult {
    RustApngOpenResult {
        status: RustApngOpenStatus::Error,
        ..open_result_defaults()
    }
}

fn read_end() -> RustApngFrameResult {
    RustApngFrameResult {
        status: RustApngReadStatus::End,
        ..read_result_defaults()
    }
}

fn read_error() -> RustApngFrameResult {
    RustApngFrameResult {
        status: RustApngReadStatus::Error,
        ..read_result_defaults()
    }
}

fn read_result_defaults() -> RustApngFrameResult {
    RustApngFrameResult {
        status: RustApngReadStatus::End,
        width: 0,
        height: 0,
        x_offset: 0,
        y_offset: 0,
        delay_num: 0,
        delay_den: 0,
        dispose_op: RustApngDisposeOp::None,
        blend_op: RustApngBlendOp::Source,
        row_bytes: 0,
        has_more_frames: false,
        pixels: Vec::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use png::{BitDepth, BlendOp, ColorType, DisposeOp, Encoder};

    const RED: &[u8] = &[255, 0, 0, 255];
    const BLUE: &[u8] = &[0, 0, 255, 255];
    const GREEN: &[u8] = &[0, 255, 0, 255];
    const TRANSLUCENT_BLUE: &[u8] = &[0, 0, 255, 128];

    #[derive(Clone)]
    struct TestFrame {
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

    impl TestFrame {
        fn full_canvas(pixel: &[u8]) -> Self {
            Self {
                width: 1,
                height: 1,
                x_offset: 0,
                y_offset: 0,
                delay_num: 1,
                delay_den: 10,
                dispose_op: DisposeOp::None,
                blend_op: BlendOp::Source,
                pixels: pixel.to_vec(),
            }
        }
    }

    fn encode_plain_png(pixel: &[u8]) -> Vec<u8> {
        let mut output = Vec::new();
        let mut encoder = Encoder::new(&mut output, 1, 1);
        encoder.set_color(ColorType::Rgba);
        encoder.set_depth(BitDepth::Eight);
        encoder
            .write_header()
            .unwrap()
            .write_image_data(pixel)
            .unwrap();
        output
    }

    fn encode_apng(
        frames: &[TestFrame],
        play_count: u32,
        hidden_default: Option<&[u8]>,
    ) -> Vec<u8> {
        let mut output = Vec::new();
        let mut encoder = Encoder::new(&mut output, 1, 1);
        encoder.set_color(ColorType::Rgba);
        encoder.set_depth(BitDepth::Eight);
        encoder
            .set_animated(u32::try_from(frames.len()).unwrap(), play_count)
            .unwrap();
        encoder.set_sep_def_img(hidden_default.is_some()).unwrap();

        let mut writer = encoder.write_header().unwrap();
        if let Some(default_pixels) = hidden_default {
            writer.write_image_data(default_pixels).unwrap();
        }

        for frame in frames {
            writer
                .set_frame_dimension(frame.width, frame.height)
                .unwrap();
            writer
                .set_frame_position(frame.x_offset, frame.y_offset)
                .unwrap();
            writer
                .set_frame_delay(frame.delay_num, frame.delay_den)
                .unwrap();
            writer.set_dispose_op(frame.dispose_op).unwrap();
            writer.set_blend_op(frame.blend_op).unwrap();
            writer.write_image_data(&frame.pixels).unwrap();
        }

        writer.finish().unwrap();
        output
    }

    fn open(data: &[u8]) -> (Box<RustApngAnimationReader>, RustApngOpenResult) {
        let mut reader = rust_new_apng_animation_reader();
        let result = rust_open_apng_animation_reader(&mut reader, data);
        (reader, result)
    }

    #[test]
    fn non_png_and_plain_png_return_not_apng() {
        let (_, result) = open(b"not png");
        assert_eq!(result.status, RustApngOpenStatus::NotApng);

        let png = encode_plain_png(RED);
        let (_, result) = open(&png);
        assert_eq!(result.status, RustApngOpenStatus::NotApng);
    }

    #[test]
    fn sequential_frames_and_loop_count_are_reported() {
        let mut second = TestFrame::full_canvas(BLUE);
        second.delay_num = 2;
        let apng = encode_apng(&[TestFrame::full_canvas(RED), second], 2, None);

        let (mut reader, result) = open(&apng);
        assert_eq!(result.status, RustApngOpenStatus::Success);
        assert_eq!(result.canvas_width, 1);
        assert_eq!(result.canvas_height, 1);
        assert_eq!(result.frame_count, 2);
        assert_eq!(result.loop_count, 1);

        let first = rust_read_apng_animation_frame(&mut reader);
        assert_eq!(first.status, RustApngReadStatus::Frame);
        assert_eq!(first.delay_num, 1);
        assert_eq!(first.delay_den, 10);
        assert_eq!(&first.pixels, RED);
        assert!(first.has_more_frames);

        let second = rust_read_apng_animation_frame(&mut reader);
        assert_eq!(second.status, RustApngReadStatus::Frame);
        assert_eq!(second.delay_num, 2);
        assert_eq!(second.delay_den, 10);
        assert_eq!(&second.pixels, BLUE);
        assert!(!second.has_more_frames);

        assert_eq!(
            rust_read_apng_animation_frame(&mut reader).status,
            RustApngReadStatus::End
        );
    }

    #[test]
    fn separate_default_image_is_skipped() {
        let hidden_default = [255, 0, 0, 255];
        let frame = TestFrame::full_canvas(BLUE);
        let apng = encode_apng(&[frame], 0, Some(&hidden_default));

        let (mut reader, result) = open(&apng);
        assert_eq!(result.status, RustApngOpenStatus::Success);
        assert_eq!(result.loop_count, -1);

        let frame = rust_read_apng_animation_frame(&mut reader);
        assert_eq!(frame.status, RustApngReadStatus::Frame);
        assert_eq!(&frame.pixels, BLUE);
        assert!(!frame.has_more_frames);
    }

    #[test]
    fn frame_control_operations_are_reported() {
        let mut over = TestFrame::full_canvas(TRANSLUCENT_BLUE);
        over.blend_op = BlendOp::Over;
        let mut background = TestFrame::full_canvas(GREEN);
        background.dispose_op = DisposeOp::Background;
        let mut previous = TestFrame::full_canvas(RED);
        previous.dispose_op = DisposeOp::Previous;
        previous.delay_den = 0;
        let apng = encode_apng(&[over, background, previous], 1, None);

        let (mut reader, result) = open(&apng);
        assert_eq!(result.status, RustApngOpenStatus::Success);

        let over = rust_read_apng_animation_frame(&mut reader);
        assert_eq!(over.blend_op, RustApngBlendOp::Over);

        let background = rust_read_apng_animation_frame(&mut reader);
        assert_eq!(background.dispose_op, RustApngDisposeOp::Background);

        let previous = rust_read_apng_animation_frame(&mut reader);
        assert_eq!(previous.dispose_op, RustApngDisposeOp::Previous);
        assert_eq!(previous.delay_num, 1);
        assert_eq!(previous.delay_den, 0);
    }

    #[test]
    fn malformed_apng_returns_error() {
        let apng = encode_apng(&[TestFrame::full_canvas(RED)], 1, None);
        let truncated = &apng[..apng.len() - 12];

        let (mut reader, result) = open(truncated);
        if result.status == RustApngOpenStatus::Success {
            assert_eq!(
                rust_read_apng_animation_frame(&mut reader).status,
                RustApngReadStatus::Error
            );
        } else {
            assert_eq!(result.status, RustApngOpenStatus::Error);
        }
    }
}
