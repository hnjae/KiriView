// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use std::{env, process};

fn startup_source() -> Result<kiriview::ApplicationStartupSource, kiriview::StartupArgumentError> {
    let working_directory = env::current_dir().ok();
    let source = kiriview::initial_source_from_args(env::args_os(), working_directory.as_deref())?;
    Ok(kiriview::application_startup_source(source))
}

fn main() {
    let startup_source = match startup_source() {
        Ok(source) => source,
        Err(error) => {
            eprintln!("KiriView: {error}");
            process::exit(kiriview::STARTUP_ARGUMENT_ERROR_EXIT_CODE);
        }
    };

    process::exit(kiriview::run_application(&startup_source));
}
