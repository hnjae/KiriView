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
mod imagedocumentsourceloadpolicy;
mod imageformatregistry;
mod imageinputclassification;
mod imagemath;
mod imagenavigationmodel;
mod imageopenworkflow;
mod imagerendergeometry;
mod imagerotation;
mod imagespreadgeometry;
mod imagespreadnavigation;
mod imagetilegeometry;
mod imageviewportgeometry;
mod imagezoomstate;
mod navigationindex;
mod predecodecachepolicy;
mod startup_arguments;
mod svgrenderer;

pub use startup_arguments::{
    STARTUP_ARGUMENT_ERROR_EXIT_CODE, StartupArgumentError, StartupSource, initial_source_from_args,
};

// Keep CXX-Qt dependency initializers linked for Rust test binaries even when Rust does not
// directly construct Qt runtime objects.
#[used]
static CXX_QT_INITIALIZER_LINK_ANCHOR: fn() = link_cxx_qt_initializer_dependencies;

fn link_cxx_qt_initializer_dependencies() {
    let _ = core::mem::size_of::<cxx_qt_lib::QString>();
}

pub use applicationruntime::{
    ApplicationStartupSource, ApplicationStartupSourceKind, application_startup_source,
    run_application,
};
