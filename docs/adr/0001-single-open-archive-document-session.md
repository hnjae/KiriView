<!--
SPDX-FileCopyrightText: 2026 KIM Hyunjae
SPDX-License-Identifier: AGPL-3.0-or-later
-->

# Single-Open Archive Document Session

## Context

Directly opened local archives are document containers. Listing pages, opening
the selected page, page navigation, and predecode all operate on the same
archive document while it remains the current document.

The older dependency shape exposed archive listing and archive image reads as
independent jobs. That made each operation free to reopen the archive backend,
which is wasteful for KArchive formats and especially poor for libarchive
formats that only support sequential access.

## Decision

The C++ document runtime owns an `ArchiveDocumentSessionStore` for default
archive access. The store tracks the current `ArchiveDocumentLocation`, caches
the sorted candidate list, and routes archive image reads through one serialized
session runner.

Backend sessions are owned below `archivebackend`:

- KArchive sessions keep the opened `KArchive` object for later candidate and
  image reads.
- Libarchive sessions open the local archive once at the filesystem file
  descriptor level for the session lifetime. They perform one sequential scan
  that stores supported image candidates, normalized entry paths, and archive
  entry order metadata, but no image payload bytes.
- A libarchive `archive*` reader is treated as a disposable sequential cursor.
  When a read can continue forward from the current cursor, the session keeps
  using it. When a backward or random read needs an earlier entry, or the cursor
  has reached EOF, the session frees the reader, seeks the held file descriptor
  back to the beginning, creates a fresh libarchive reader on that same file
  descriptor, and replays entries until the requested image is reached.

The session is discarded when the document runtime leaves that archive document:
opening a different archive document, opening a non-archive image, clearing the
document, successfully deleting the current document, or shutting down.

## Consequences

Archive document navigation and predecode reuse the candidate list and archive
storage already opened for the current document. This keeps the archive backend
open count to one filesystem-level open per current archive document session in
the default runtime path.

Libarchive sessions avoid session-owned memory or temporary-file payload caches.
The tradeoff is that backward and random reads are O(n) replays through the
archive from the beginning. The requested image still exists transiently in the
returned image data buffer.

Injected test or development dependencies can still bypass the runtime store;
that preserves focused controller tests. Tests that need archive-session
semantics can inject an instrumented archive session factory.
