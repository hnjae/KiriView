// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum NavigationDirection {
    Previous,
    Next,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct NavigationIndex {
    pub(crate) found: bool,
    pub(crate) index: usize,
}

pub(crate) fn adjacent_navigation_index(
    candidate_count: usize,
    current: NavigationIndex,
    direction: NavigationDirection,
) -> NavigationIndex {
    if !current.found || current.index >= candidate_count {
        return missing_index();
    }

    match direction {
        NavigationDirection::Previous => {
            if current.index == 0 {
                missing_index()
            } else {
                found_index(current.index - 1)
            }
        }
        NavigationDirection::Next => {
            let next_index = current.index + 1;
            if next_index == candidate_count {
                missing_index()
            } else {
                found_index(next_index)
            }
        }
    }
}

pub(crate) fn navigation_index_for_matches<I>(matches: I) -> NavigationIndex
where
    I: IntoIterator<Item = bool>,
{
    for (index, matches_current) in matches.into_iter().enumerate() {
        if matches_current {
            return found_index(index);
        }
    }

    missing_index()
}

pub(crate) fn found_index(index: usize) -> NavigationIndex {
    NavigationIndex { found: true, index }
}

pub(crate) fn missing_index() -> NavigationIndex {
    NavigationIndex {
        found: false,
        index: 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn adjacent_navigation_index_moves_without_wrapping() {
        assert_eq!(
            adjacent_navigation_index(3, found_index(1), NavigationDirection::Previous),
            found_index(0)
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(1), NavigationDirection::Next),
            found_index(2)
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(0), NavigationDirection::Previous),
            missing_index()
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(2), NavigationDirection::Next),
            missing_index()
        );
    }

    #[test]
    fn adjacent_navigation_index_rejects_missing_or_out_of_range_current() {
        assert_eq!(
            adjacent_navigation_index(3, missing_index(), NavigationDirection::Next),
            missing_index()
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(3), NavigationDirection::Previous),
            missing_index()
        );
    }

    #[test]
    fn navigation_index_for_matches_uses_first_match() {
        assert_eq!(
            navigation_index_for_matches([false, true, true]),
            found_index(1)
        );
        assert_eq!(
            navigation_index_for_matches([false, false]),
            missing_index()
        );
    }
}
