// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

mod apngdecoder;
mod archivepath;
mod avifcompat;
mod bmff;
mod byteio;
mod filedeletionfallback;
mod heifcontainer;
mod imagebytecost;
mod imagecontainer;
mod imagedocumentstate;
mod imageformatregistry;
mod imagemath;
mod imagenavigationmodel;
mod imageopenworkflow;
mod imagespreadgeometry;
mod imagetilegeometry;
mod imageurl;
mod imageviewportgeometry;
mod imagezoomstate;
mod localization;
mod predecodecachepolicy;
mod startup_arguments;

pub use startup_arguments::{
    STARTUP_ARGUMENT_ERROR_EXIT_CODE, StartupArgumentError, initial_source_url_from_args,
};

pub fn initialize_rust_modules() {
    let _ = core::mem::size_of::<cxx_qt_lib_extras::QApplication>();
}

pub use localization::{initialize_localization, setup_localized_context};
