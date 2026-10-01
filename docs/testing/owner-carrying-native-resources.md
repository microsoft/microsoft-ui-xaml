# Owner-carrying native resources

`DependentResource<Owner, Resource, Traits>` stores a native pointer together with
an `xref_ptr` to the owner required for its use or cleanup. It uses existing
`AddRef`/`Release`; it does not introduce a reference counter, managed tracking
edge, or global registry.

## Contract

- Only the resource-specific traits can adopt a nonempty pair. The adapter must
  establish provenance while the producer still protects the original context.
- Adoption clears the producer's actual cleanup slot, not a copy of that slot.
  A local holder protects the handoff if allocating the outer wrapper fails.
- Copies are disabled. Moves transfer both fields and clear the source.
- Reset removes the old pair from the holder before calling external cleanup.
  The old owner remains retained until cleanup returns.
- Move assignment installs the incoming pair before destroying the old pair.
  A replacement installed by reentrant cleanup is not overwritten.
- A null pointer does not invoke resource cleanup. An adapter can retain an owner
  for a valid zero-length view, or return a default holder when no obligation
  exists at all.
- `Get()` returns a borrow, not another cleanup owner. The owning wrapper must
  remain alive and unreset for the entire borrow.
- Traits cleanup must be nonthrowing and consuming. APIs that may fail while
  leaving a resource live need an explicit close protocol, not silent disposal.

This holder is for pointer resources whose empty cleanup value is `nullptr`.
It does not impose that convention on other kinds of native handles.

## Migrated families

| Family | Resource and required owner | Integration |
| --- | --- | --- |
| Line Services cached break | `PLSBREAKRECLINE` and its originating `LsTextFormatter` | `LsTextLine::CreateLineBreak` takes the producer slot by reference, adopts into a guard, then moves into `LsTextLineBreak`. The wrapper no longer accepts a raw context/record pair. |
| WIC pixel view | Buffer address and `IWICBitmapLock` | `WicBitmapLock` acquires the address from the same lock it retains. Moving clears the source view; `Unlock` clears the view and releases the lock. |
| SoftwareBitmap pixel view | Buffer address and `IMemoryBufferReference` | `SoftwareBitmapLock` binds the address obtained through byte access to that same reference. The public bitmap can be released while the lock wrapper remains alive. |

The imaging wrappers already retained their dependencies before this change.
Their migration makes that relationship one move-only value rather than two
independently maintained fields; it is hardening, not evidence of another crash.
Pixels are borrowed, so the imaging adapters do not free them. Releasing the
retained lock/reference ends the view's lifetime.

The LS formatter's context is created once and released by its destructor;
`ReleaseLsContext` is private and its only caller is that destructor. Retaining
that formatter therefore protects the original context against formatter-pool
eviction. `LsBreakRecordTraits` has narrow access to this private state and
fail-fasts if LS reports that record cleanup failed, rather than dropping the
dependency and continuing with an unconsumed resource.

`ParagraphNode` and exported paragraph breaks still share the existing
reference-counted `LsTextLineBreak`. They automatically retain the complete pair;
their storage and reference-counting contracts do not change. Returning a
formatter checkout still does not pin it in the pool's used list. Pool trimming
can remove its own reference while surviving break wrappers retain theirs.

## Other audited families

| Family | Disposition |
| --- | --- |
| `LsTextLine` / LS rendered line | Already retains its formatter and destroys the native line before releasing it. No redundant owner was added. |
| LS embedded object/context allocations | Owned and destroyed through LS object callbacks. Adding references from those context-owned children back to the formatter risks a cycle; not a missing cached-break ownership edge. |
| `DWriteFontFace::TryGetFontTable` | Returns `E_UNEXPECTED` in this implementation. No active table-token acquisition path to migrate was found here. |
| `SystemMemoryBitsDriver` mappings | Already retain texture and lockable device-context state. Preserve the graphics-lock and mapping protocol. |
| `ByteAccessDxgiSurface` / diagnostic DXGI maps | Device-loss checks, graphics locks and fallible `Unmap` require an explicit mapping-close contract. Not converted to a generic consuming destructor. |
| COM/reference-tracked UI edges | Not this helper's contract. Continue using the appropriate tracking and teardown mechanisms. |

This is a bounded audit of these families, not a claim that every native handle
in WinUI has been inspected or that all context-lifetime bugs are fixed.

## Coverage and limits

`DependentResourceUnitTests` runs in the existing isolated Base test project.
It covers shared origins, cleared producer slots, moved-from state, self-move,
occupied destinations, repeated and reentrant reset, reentrant replacement during
move assignment, abandoned factory guards, empty results and null buffer views.

`OfferableSoftwareBitmapUnitTests` in the isolated Foundation.Imaging project
uses real WIC/SoftwareBitmap buffers to cover moves, release of the caller's bitmap,
unlock/relock, and empty WIC rectangles.

The LS ownership probe used during development failed both draw/trim/evict orders
before the change and passed with the new adapter/wrapper. It instrumented LS
context allocation/destruction and completed-line setup; it was not a real LS
formatting or application-crash reproduction.

Before landing, exercise real LS formatting/drawing with both trim orders,
exported paragraph breaks, wrapping retries, empty/end conditions, and allocation
failure. Check navigation/low-memory retention, correct-thread cleanup and core
shutdown. Retaining a formatter does not itself preserve independently closing
services, grant an exclusive pool checkout, or establish cross-context
continuation compatibility. No crash-specific double-free attribution is made.
