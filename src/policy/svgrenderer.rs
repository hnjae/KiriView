// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use resvg::{
    tiny_skia::{Pixmap, Transform},
    usvg::{ImageHrefResolver, Options, Tree},
};

#[cxx::bridge]
mod ffi {
    #[namespace = "kiriview"]
    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustSvgImageSize {
        valid: bool,
        width: i32,
        height: i32,
    }

    #[namespace = "kiriview"]
    extern "Rust" {
        #[cxx_name = "rustSvgIntrinsicSize"]
        fn rust_svg_intrinsic_size(data: &[u8]) -> RustSvgImageSize;

        #[cxx_name = "rustRenderSvgImage"]
        fn rust_render_svg_image(data: &[u8], width: i32, height: i32) -> Vec<u8>;

        #[cxx_name = "rustRenderSvgTile"]
        fn rust_render_svg_tile(
            data: &[u8],
            level_width: i32,
            level_height: i32,
            texture_x: i32,
            texture_y: i32,
            texture_width: i32,
            texture_height: i32,
        ) -> Vec<u8>;
    }
}

use ffi::RustSvgImageSize;

fn rust_svg_intrinsic_size(data: &[u8]) -> RustSvgImageSize {
    svg_intrinsic_size(data)
}

fn rust_render_svg_image(data: &[u8], width: i32, height: i32) -> Vec<u8> {
    render_svg_image(data, width, height).unwrap_or_default()
}

fn rust_render_svg_tile(
    data: &[u8],
    level_width: i32,
    level_height: i32,
    texture_x: i32,
    texture_y: i32,
    texture_width: i32,
    texture_height: i32,
) -> Vec<u8> {
    render_svg_tile(
        data,
        level_width,
        level_height,
        texture_x,
        texture_y,
        texture_width,
        texture_height,
    )
    .unwrap_or_default()
}

fn svg_intrinsic_size(data: &[u8]) -> RustSvgImageSize {
    let Some(tree) = parse_svg_tree(data) else {
        return invalid_svg_image_size();
    };

    rust_svg_image_size(
        tree.size().to_int_size().width(),
        tree.size().to_int_size().height(),
    )
}

fn render_svg_image(data: &[u8], width: i32, height: i32) -> Option<Vec<u8>> {
    let (width, height) = positive_dimensions(width, height)?;
    let tree = parse_svg_tree(data)?;
    let transform = image_transform(&tree, width, height);

    render_tree(&tree, width, height, transform)
}

fn render_svg_tile(
    data: &[u8],
    level_width: i32,
    level_height: i32,
    texture_x: i32,
    texture_y: i32,
    texture_width: i32,
    texture_height: i32,
) -> Option<Vec<u8>> {
    let (level_width, level_height) = positive_dimensions(level_width, level_height)?;
    let (texture_width, texture_height) = positive_dimensions(texture_width, texture_height)?;
    let tree = parse_svg_tree(data)?;
    let svg_size = tree.size();
    let transform = Transform::from_row(
        level_width as f32 / svg_size.width(),
        0.0,
        0.0,
        level_height as f32 / svg_size.height(),
        -texture_x as f32,
        -texture_y as f32,
    );

    render_tree(&tree, texture_width, texture_height, transform)
}

fn parse_svg_tree(data: &[u8]) -> Option<Tree> {
    let options = svg_options();
    Tree::from_data(data, &options).ok()
}

fn svg_options() -> Options<'static> {
    Options {
        resources_dir: None,
        image_href_resolver: ImageHrefResolver {
            resolve_data: ImageHrefResolver::default_data_resolver(),
            resolve_string: Box::new(|_, _| None),
        },
        ..Options::default()
    }
}

fn image_transform(tree: &Tree, width: u32, height: u32) -> Transform {
    let svg_size = tree.size();
    Transform::from_scale(
        width as f32 / svg_size.width(),
        height as f32 / svg_size.height(),
    )
}

fn render_tree(tree: &Tree, width: u32, height: u32, transform: Transform) -> Option<Vec<u8>> {
    let mut pixmap = Pixmap::new(width, height)?;
    resvg::render(tree, transform, &mut pixmap.as_mut());
    Some(pixmap.take())
}

fn positive_dimensions(width: i32, height: i32) -> Option<(u32, u32)> {
    Some((u32::try_from(width).ok()?, u32::try_from(height).ok()?))
        .filter(|(width, height)| *width > 0 && *height > 0 && width.checked_mul(*height).is_some())
}

fn rust_svg_image_size(width: u32, height: u32) -> RustSvgImageSize {
    let Some(width) = i32::try_from(width).ok() else {
        return invalid_svg_image_size();
    };
    let Some(height) = i32::try_from(height).ok() else {
        return invalid_svg_image_size();
    };

    RustSvgImageSize {
        valid: true,
        width,
        height,
    }
}

fn invalid_svg_image_size() -> RustSvgImageSize {
    RustSvgImageSize {
        valid: false,
        width: 0,
        height: 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const SVG_WITH_CLIP_PATH: &[u8] =
        br##"<svg xmlns="http://www.w3.org/2000/svg" width="12" height="8">
<rect width="12" height="8" fill="white"/>
<clipPath id="clip"><rect x="2" y="1" width="4" height="4"/></clipPath>
<g clip-path="url(#clip)"><rect width="12" height="8" fill="red"/></g>
</svg>"##;

    #[test]
    fn reports_valid_intrinsic_size() {
        let size = svg_intrinsic_size(
            br#"<svg xmlns="http://www.w3.org/2000/svg" width="80" height="40"/>"#,
        );

        assert_eq!(
            size,
            RustSvgImageSize {
                valid: true,
                width: 80,
                height: 40,
            }
        );
    }

    #[test]
    fn rejects_invalid_svg_data() {
        assert!(!svg_intrinsic_size(b"not svg").valid);
        assert!(render_svg_image(b"not svg", 12, 8).is_none());
    }

    #[test]
    fn renders_clip_path_to_premultiplied_rgba_bytes() {
        let bytes =
            render_svg_image(SVG_WITH_CLIP_PATH, 12, 8).expect("clip-path SVG should render");

        assert_eq!(bytes.len(), 12 * 8 * 4);
        assert_eq!(pixel(&bytes, 12, 3, 2), [255, 0, 0, 255]);
        assert_eq!(pixel(&bytes, 12, 7, 2), [255, 255, 255, 255]);
    }

    fn pixel(bytes: &[u8], width: usize, x: usize, y: usize) -> [u8; 4] {
        let offset = (y * width + x) * 4;
        bytes[offset..offset + 4]
            .try_into()
            .expect("test pixel should be in bounds")
    }
}
