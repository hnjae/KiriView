// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

mod apngdecoder;
mod avifcompat;
mod imageviewportgeometry;

pub fn initialize_rust_modules() {
    let _ = core::mem::size_of::<cxx_qt_lib_extras::QApplication>();
}
