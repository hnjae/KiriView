// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

mod applicationruntime;
mod archiveformat;
mod archivepath;
mod avifcompat;
mod bmff;
mod byteio;
mod cachebudget;
mod heifcontainer;
mod imagecontainer;
mod imagedeletionpolicy;
mod imageformatregistry;
mod imagemath;
mod imagenavigationmodel;
mod imageopenworkflow;
mod imagerendergeometry;
mod imagespreadgeometry;
mod imagespreadnavigation;
mod imagetilegeometry;
mod imageviewportgeometry;
mod imagezoomstate;
mod navigationindex;
mod predecodecachepolicy;
mod startup_arguments;

pub use startup_arguments::{
    STARTUP_ARGUMENT_ERROR_EXIT_CODE, StartupArgumentError, StartupSource, initial_source_from_args,
};

pub fn initialize_rust_modules() {
    let _ = core::mem::size_of::<cxx_qt_lib_extras::QApplication>();
}

pub use applicationruntime::{
    initial_source_url_from_local_file_path, initial_source_url_from_url_text,
    initialize_application_runtime, load_application_main_qml,
};
