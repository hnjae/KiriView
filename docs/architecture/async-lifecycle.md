# Async Lifecycle

Out-of-order workflows must have one local lifecycle owner. The owner is the only object allowed to accept a completion and publish public state from it.

Every async owner contract names its owner token, QObject affinity, queued acceptance path, identity kind, destruction invalidation point, cancellation behavior, and result publication point.

The identity kind is one of operation id, scoped operation id, source key plus freshness generation, demand key, or display-source revision. A completion must carry that identity back to the owner, and the owner must accept or reject the identity before mutating authoritative state.

Cancellation is either guaranteed or best-effort. Guaranteed cancellation means no completion callback can run after cancel returns. Best-effort cancellation means the owner invalidates identity and treats any later callback as stale or no-op. Qt and KIO jobs are usually best-effort unless the provider contract explicitly proves otherwise.

QObject-backed work must use a live owner token or weak Qt reference before queued delivery can run. Owner destruction must disconnect external signals where the owner owns the connection, cancel active jobs, invalidate operation identity, and leave queued callbacks unable to dereference the destroyed owner.

Worker-thread and Rust async work return plain payloads only. Payloads crossing a worker boundary must be safe to move to the GUI acceptance path, must not contain QObject pointers, and must not carry an alternate authoritative URL, page, frame, or status beside the owner's current identity.

Worker-backed owners must receive scheduling through an injectable dependency port when tests need deterministic lifecycle control. Production adapters may use the global Qt thread pool, but runtime owners such as image decode jobs should depend on a scheduler contract that preserves QObject-guarded queued delivery and stale-completion acceptance instead of calling global worker primitives directly.

QML may own UI-local timers and physical item transients, but it must not use delayed callbacks to reconcile durable domain state. If a delayed UI callback observes public session state, the C++ owner must already have published a coherent snapshot for that state.

Use the existing support vocabulary where it fits: `ImageAsyncTicket` for generation acceptance, `ImageAsyncOperationState` for single operation ownership, `ImageAsyncScopedOperationState` for operation plus scope ownership, and `ImageIoJob` for cancelable QObject/job lifetime. Do not introduce a shared base state machine unless several owners need the same behavior and the abstraction removes real duplicated lifecycle logic.
