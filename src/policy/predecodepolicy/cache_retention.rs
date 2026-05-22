// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use super::RustPredecodeCachedImageState;

#[derive(Clone, Copy)]
struct RetainedCachedImage {
    original_index: usize,
    current_displayed: bool,
    recent_displayed: bool,
    current_displayed_priority: usize,
    recent_displayed_priority: usize,
    window_priority: usize,
    byte_cost: i64,
}

pub(super) fn retained_cached_image_indices(
    states: Vec<RustPredecodeCachedImageState>,
    window_count: usize,
    byte_budget: i64,
) -> Vec<usize> {
    if byte_budget <= 0 {
        return Vec::new();
    }

    let mut images: Vec<RetainedCachedImage> = states
        .into_iter()
        .enumerate()
        .filter_map(|(original_index, state)| {
            if (!state.current_displayed
                && !state.recent_displayed
                && state.window_priority >= window_count)
                || state.byte_cost <= 0
            {
                return None;
            }

            Some(RetainedCachedImage {
                original_index,
                current_displayed: state.current_displayed,
                recent_displayed: state.recent_displayed,
                current_displayed_priority: state.current_displayed_priority,
                recent_displayed_priority: state.recent_displayed_priority,
                window_priority: state.window_priority,
                byte_cost: state.byte_cost,
            })
        })
        .collect();

    let mut current_displayed_images: Vec<RetainedCachedImage> = images
        .iter()
        .copied()
        .filter(|image| image.current_displayed)
        .collect();
    current_displayed_images.sort_by(|left, right| {
        left.current_displayed_priority
            .cmp(&right.current_displayed_priority)
            .then(left.original_index.cmp(&right.original_index))
    });

    let current_displayed_byte_cost = current_displayed_images
        .iter()
        .fold(0_i64, |total, image| total.saturating_add(image.byte_cost));
    let adjacent_byte_budget = byte_budget.saturating_sub(current_displayed_byte_cost);

    images.retain(|image| !image.current_displayed);
    images.sort_by(|left, right| {
        retention_group(*left)
            .cmp(&retention_group(*right))
            .then_with(|| retention_priority(*left).cmp(&retention_priority(*right)))
            .then(left.original_index.cmp(&right.original_index))
    });

    let mut retained_images = Vec::new();
    let mut retained_byte_cost = 0_i64;
    for image in images {
        if retained_byte_cost.saturating_add(image.byte_cost) > adjacent_byte_budget {
            break;
        }

        retained_byte_cost = retained_byte_cost.saturating_add(image.byte_cost);
        retained_images.push(image);
    }

    current_displayed_images.extend(retained_images);
    current_displayed_images
        .into_iter()
        .map(|image| image.original_index)
        .collect()
}

fn retention_group(image: RetainedCachedImage) -> usize {
    if image.recent_displayed { 0 } else { 1 }
}

fn retention_priority(image: RetainedCachedImage) -> usize {
    if image.recent_displayed {
        image.recent_displayed_priority
    } else {
        image.window_priority
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn retained_cached_image_indices_drop_images_outside_window_and_sort_by_priority() {
        assert_eq!(
            retained_cached_image_indices(
                vec![
                    cached_image_state(2, 10),
                    cached_image_state(3, 10),
                    cached_image_state(0, 10),
                    cached_image_state(1, 10),
                ],
                3,
                100,
            ),
            vec![2, 3, 0]
        );
    }

    #[test]
    fn retained_cached_image_indices_trim_lowest_priority_images_to_budget() {
        assert_eq!(
            retained_cached_image_indices(
                vec![
                    cached_image_state(2, 50),
                    cached_image_state(0, 60),
                    cached_image_state(1, 60),
                ],
                3,
                120,
            ),
            vec![1, 2]
        );
    }

    #[test]
    fn retained_cached_image_indices_preserve_priority_prefix_when_trimming() {
        assert_eq!(
            retained_cached_image_indices(
                vec![
                    cached_image_state(0, 100),
                    cached_image_state(1, 100),
                    cached_image_state(2, 1),
                ],
                3,
                101,
            ),
            vec![0]
        );
    }

    #[test]
    fn retained_cached_image_indices_keep_displayed_images_before_adjacent_budget() {
        assert_eq!(
            retained_cached_image_indices(
                vec![
                    cached_image_state(0, 60),
                    displayed_cached_image_state(1, 90),
                    cached_image_state(2, 60),
                    displayed_cached_image_state(3, 90),
                ],
                4,
                160,
            ),
            vec![1, 3]
        );
    }

    #[test]
    fn retained_cached_image_indices_keep_displayed_images_without_window() {
        assert_eq!(
            retained_cached_image_indices(
                vec![
                    displayed_cached_image_state(usize::MAX, 90),
                    cached_image_state(0, 10),
                ],
                0,
                80,
            ),
            vec![0]
        );
    }

    #[test]
    fn retained_cached_image_indices_prioritize_current_recent_then_window() {
        assert_eq!(
            retained_cached_image_indices(
                vec![
                    cached_image_state(0, 80),
                    recent_cached_image_state(0, 80),
                    displayed_cached_image_state(0, 80),
                ],
                1,
                160,
            ),
            vec![2, 1]
        );
    }

    fn cached_image_state(window_priority: usize, byte_cost: i64) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            current_displayed: false,
            recent_displayed: false,
            current_displayed_priority: usize::MAX,
            recent_displayed_priority: usize::MAX,
            window_priority,
            byte_cost,
        }
    }

    fn displayed_cached_image_state(
        window_priority: usize,
        byte_cost: i64,
    ) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            current_displayed: true,
            recent_displayed: false,
            current_displayed_priority: window_priority,
            recent_displayed_priority: usize::MAX,
            window_priority,
            byte_cost,
        }
    }

    fn recent_cached_image_state(
        recent_displayed_priority: usize,
        byte_cost: i64,
    ) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            current_displayed: false,
            recent_displayed: true,
            current_displayed_priority: usize::MAX,
            recent_displayed_priority,
            window_priority: usize::MAX,
            byte_cost,
        }
    }
}
