# Async Lifecycle

Out-of-order workflows must have one local lifecycle owner. The owner is the only object allowed to accept a completion and publish public state from it.

Every async owner contract names its owner token, QObject affinity, queued acceptance path, identity kind, destruction invalidation point, cancellation behavior, and result publication point.

The identity kind is one of operation id, scoped operation id, source key plus freshness generation, demand key, or display-source revision. A completion must carry that identity back to the owner, and the owner must accept or reject the identity before mutating authoritative state.

Cancellation is either guaranteed or best-effort. Guaranteed cancellation means no completion callback can run after cancel returns. Best-effort cancellation means the owner invalidates identity and treats any later callback as stale or no-op. Qt and KIO jobs are usually best-effort unless the provider contract explicitly proves otherwise.

QObject-backed work must use a live owner token or weak Qt reference before queued delivery can run. Owner destruction must disconnect external signals where the owner owns the connection, cancel active jobs, invalidate operation identity, and leave queued callbacks unable to dereference the destroyed owner.

Worker-thread and Rust async work return plain payloads only. Payloads crossing a worker boundary must be safe to move to the GUI acceptance path, must not contain QObject pointers, and must not carry an alternate authoritative URL, page, frame, or status beside the owner's current identity.

Worker-backed owners must receive scheduling through an injectable dependency port when tests need deterministic lifecycle control. Production adapters may use the global Qt thread pool, but runtime owners such as image decode jobs and opened-collection media entry runtimes must depend on a scheduler contract that preserves QObject-guarded queued delivery and stale-completion acceptance instead of calling global worker primitives directly.

Default worker-backed providers filled from a dependency struct must capture that struct's resolved scheduler when one is present. Resolving a default data loader, opened-collection candidate loader, thumbnail cache lookup, or thumbnail generation provider must not silently bypass the scheduler dependency carried by the consuming runtime.

Shared async support owns reusable callback delivery, cancelable job wrappers, operation-state helpers, directory-listing provider contracts, and worker scheduling primitives. Default domain loaders must live with the owning domain: image decode data loading belongs to decoding, page-candidate loading belongs to navigation, opened-collection byte access belongs to collection media-entry owners, and thumbnail byte or cache work belongs to thumbnail runtime adapters.

Session runtime dependencies own active-navigation thumbnail worker scheduling. If the session resolves default thumbnail lookup or generation providers, it must bind them to the session's thumbnail worker scheduler before constructing the thumbnail runtime.

Image page-surface owners must schedule raster display refinement through an injected worker scheduler. Refinement completions carry the display-source demand key back to the page-surface owner, which accepts or rejects them before publishing a new provider entry.

Timer-backed owners must receive monotonic time and timer firing through dependency ports when behavior depends on elapsed time. Production adapters may use `QElapsedTimer` and `QTimer`, but runtime state must consume plain timestamps and scheduled callback events so tests can advance time or fire callbacks without waiting on wall-clock delays.

Directory listing and watching must cross an injectable provider boundary. Production providers may use `KCoreDirLister`, KDirNotify, and KIO jobs, but candidate loaders, direct-media navigation, and live candidate stores must consume directory snapshots, change events, deletion events, and listing failures through provider ports so core navigation behavior can be tested without real filesystem or KDE notification timing. Live image page candidate entries own subscriber and pending-load state, but they must not construct or connect KDE listers directly.

QML may own UI-local timers and physical item transients, but it must not use delayed callbacks to reconcile durable domain state. If a delayed UI callback observes public session state, the C++ owner must already have published a coherent snapshot for that state.

Reusable async primitives are appropriate only when they preserve an owner's operation identity, stale-completion rejection, cancellation semantics, and QObject lifetime rules without becoming a shared authoritative state machine. Do not introduce a shared base state machine unless several owners need the same behavior and the abstraction removes real duplicated lifecycle logic.
