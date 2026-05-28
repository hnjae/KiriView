# Single-Open Media Entry Source

## Context

Directly opened local archives are opened collections. Listing pages, opening the selected page, page navigation, and predecode all operate on the same archive collection while it remains the active opened collection scope.

The older dependency shape exposed opened-collection candidate listing and image reads as independent jobs. Each operation could reopen the source backend, adding overhead for KArchive formats and increasing replay cost for libarchive formats that only support sequential access.

## Decision

The C++ document runtime owns a `MediaEntrySourceStore` for default opened-collection media entry access. The store tracks the current `OpenedCollectionScopeLocation`, caches the sorted candidate list, and routes opened-collection image reads through one serialized media-entry-source runner.

Media entry sources are owned below `mediaentrysourcebackend`:

- KArchive media entry sources keep the opened `KArchive` object for later candidate and image reads.
- Libarchive media entry sources open the local archive once at the filesystem file descriptor level for the source lifetime. They perform one sequential scan that stores supported image candidates, normalized entry paths, and archive entry order metadata, but no image payload bytes.
- A libarchive `archive*` reader is treated as a disposable sequential cursor. When a read can continue forward from the current cursor, the source keeps using it. When a backward or random read needs an earlier entry, or the cursor has reached EOF, the source frees the reader, seeks the held file descriptor back to the beginning, creates a fresh libarchive reader on that same file descriptor, and replays entries until the requested image is reached.

The source is discarded when the document runtime leaves that opened collection scope: opening a different opened collection, opening direct media, clearing the image document, successfully deleting the current collection, or shutting down.

## Consequences

Archive collection navigation and predecode reuse the candidate list and archive storage already opened for the current media entry source. This keeps archive storage open count to one filesystem-level open per active archive collection source in the default runtime path.

Libarchive media entry sources avoid source-owned memory or temporary-file payload caches. The tradeoff is that backward and random reads are O(n) replays through the archive from the beginning. The requested image still exists transiently in the returned image data buffer.

Injected test or development dependencies can still bypass the runtime store; that preserves focused controller tests. Tests that need media-entry-source semantics can inject an instrumented media entry source factory.
