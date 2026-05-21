// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::navigationindex::{
    NavigationDirection, NavigationIndex, adjacent_navigation_index, found_index, missing_index,
};

#[allow(dead_code)]
pub(crate) fn ordinary_media_candidate_index(
    sorted_candidates: &[String],
    current: &str,
) -> NavigationIndex {
    sorted_candidates
        .iter()
        .position(|candidate| candidate == current)
        .map_or_else(missing_index, found_index)
}

#[allow(dead_code)]
pub(crate) fn adjacent_ordinary_media_candidate(
    sorted_candidates: &[String],
    current: &str,
    direction: NavigationDirection,
) -> Option<String> {
    let current_index = ordinary_media_candidate_index(sorted_candidates, current);
    let target_index = adjacent_navigation_index(sorted_candidates.len(), current_index, direction);

    target_index
        .found
        .then(|| sorted_candidates[target_index.index].clone())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn candidates() -> Vec<String> {
        ["001.jpg", "002.mp4", "003.png", "004.mov"]
            .into_iter()
            .map(str::to_owned)
            .collect()
    }

    #[test]
    fn finds_current_item_in_already_sorted_candidate_vector() {
        assert_eq!(
            ordinary_media_candidate_index(&candidates(), "003.png"),
            found_index(2)
        );
    }

    #[test]
    fn returns_previous_and_next_media_without_sorting_candidates() {
        let candidates = candidates();

        assert_eq!(
            adjacent_ordinary_media_candidate(
                &candidates,
                "003.png",
                NavigationDirection::Previous
            ),
            Some("002.mp4".to_owned())
        );
        assert_eq!(
            adjacent_ordinary_media_candidate(&candidates, "003.png", NavigationDirection::Next),
            Some("004.mov".to_owned())
        );
    }

    #[test]
    fn does_not_wrap_at_boundaries() {
        let candidates = candidates();

        assert_eq!(
            adjacent_ordinary_media_candidate(
                &candidates,
                "001.jpg",
                NavigationDirection::Previous
            ),
            None
        );
        assert_eq!(
            adjacent_ordinary_media_candidate(&candidates, "004.mov", NavigationDirection::Next),
            None
        );
    }

    #[test]
    fn rejects_missing_current_item() {
        assert_eq!(
            ordinary_media_candidate_index(&candidates(), "missing.mp4"),
            missing_index()
        );
        assert_eq!(
            adjacent_ordinary_media_candidate(
                &candidates(),
                "missing.mp4",
                NavigationDirection::Next
            ),
            None
        );
    }
}
