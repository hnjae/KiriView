// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

mod apngdecoder;
mod avifcompat;
mod imagenavigationmodel;
mod imageviewportgeometry;
mod imagezoomstate;
mod startup_arguments;

pub use startup_arguments::{
    STARTUP_ARGUMENT_ERROR_EXIT_CODE, StartupArgumentError, initial_source_url_from_args,
};

pub fn initialize_rust_modules() {
    let _ = core::mem::size_of::<cxx_qt_lib_extras::QApplication>();
}
