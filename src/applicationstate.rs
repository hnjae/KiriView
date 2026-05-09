// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const HAMBURGER_MENU_PRESENTATION: i32 = 0;
const MENU_BAR_PRESENTATION: i32 = 1;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustMenuPresentationUpdate {
        changed: bool,
        config_value: i32,
    }

    extern "Rust" {
        #[cxx_name = "rustMenuPresentationFromConfigValue"]
        fn rust_menu_presentation_from_config_value(config_value: i32) -> i32;

        #[cxx_name = "rustMenuPresentationUpdate"]
        fn rust_menu_presentation_update(
            current_config_value: i32,
            requested_config_value: i32,
        ) -> RustMenuPresentationUpdate;
    }
}

use ffi::RustMenuPresentationUpdate;

fn rust_menu_presentation_from_config_value(config_value: i32) -> i32 {
    match config_value {
        MENU_BAR_PRESENTATION => MENU_BAR_PRESENTATION,
        _ => HAMBURGER_MENU_PRESENTATION,
    }
}

fn rust_menu_presentation_update(
    current_config_value: i32,
    requested_config_value: i32,
) -> RustMenuPresentationUpdate {
    RustMenuPresentationUpdate {
        changed: rust_menu_presentation_from_config_value(current_config_value)
            != requested_config_value,
        config_value: requested_config_value,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn menu_presentation_config_value_defaults_to_hamburger_menu() {
        assert_eq!(rust_menu_presentation_from_config_value(0), 0);
        assert_eq!(rust_menu_presentation_from_config_value(1), 1);
        assert_eq!(rust_menu_presentation_from_config_value(-1), 0);
        assert_eq!(rust_menu_presentation_from_config_value(99), 0);
    }

    #[test]
    fn menu_presentation_update_reports_real_changes() {
        assert_eq!(
            rust_menu_presentation_update(0, 0),
            RustMenuPresentationUpdate {
                changed: false,
                config_value: 0,
            }
        );
        assert_eq!(
            rust_menu_presentation_update(0, 1),
            RustMenuPresentationUpdate {
                changed: true,
                config_value: 1,
            }
        );
    }

    #[test]
    fn menu_presentation_update_preserves_requested_raw_config_value() {
        assert_eq!(
            rust_menu_presentation_update(0, 99),
            RustMenuPresentationUpdate {
                changed: true,
                config_value: 99,
            }
        );
        assert_eq!(
            rust_menu_presentation_update(99, 0),
            RustMenuPresentationUpdate {
                changed: false,
                config_value: 0,
            }
        );
    }
}
