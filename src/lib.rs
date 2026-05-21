// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Application policy.
#[path = "policy/applicationruntime.rs"]
mod applicationruntime;
#[path = "policy/imageactionavailability.rs"]
mod imageactionavailability;
#[path = "policy/startup_arguments.rs"]
mod startup_arguments;

// Archive policy.
#[path = "policy/archiveformat.rs"]
mod archiveformat;
#[path = "policy/archivepath.rs"]
mod archivepath;

// Cache policy.
#[path = "policy/cachebudget.rs"]
mod cachebudget;

// Decoding and format policy.
#[path = "policy/avifcompat.rs"]
mod avifcompat;
#[path = "policy/bmff.rs"]
mod bmff;
#[path = "policy/byteio.rs"]
mod byteio;
#[path = "policy/heifcontainer.rs"]
mod heifcontainer;
#[path = "policy/heiftiling.rs"]
mod heiftiling;
#[path = "policy/imageformatregistry.rs"]
mod imageformatregistry;
#[path = "policy/imageinputclassification.rs"]
mod imageinputclassification;

// Document workflow policy.
#[path = "policy/imagedocumentsourceloadpolicy.rs"]
mod imagedocumentsourceloadpolicy;
#[path = "policy/imageopenworkflow.rs"]
mod imageopenworkflow;

// Navigation policy.
#[path = "policy/imagenavigationmodel.rs"]
mod imagenavigationmodel;
#[path = "policy/navigationindex.rs"]
mod navigationindex;

// Predecode policy.
#[path = "policy/predecodecachepolicy.rs"]
mod predecodecachepolicy;

// Presentation policy.
#[path = "policy/imagemath.rs"]
mod imagemath;
#[path = "policy/imagerotation.rs"]
mod imagerotation;
#[path = "policy/imagespreadgeometry.rs"]
mod imagespreadgeometry;
#[path = "policy/imagespreadnavigation.rs"]
mod imagespreadnavigation;
#[path = "policy/imageviewportgeometry.rs"]
mod imageviewportgeometry;
#[path = "policy/imagezoomstate.rs"]
mod imagezoomstate;

// Rendering policy.
#[path = "policy/imagerendergeometry.rs"]
mod imagerendergeometry;
#[path = "policy/imagetilegeometry.rs"]
mod imagetilegeometry;
#[path = "policy/svgrenderer.rs"]
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
