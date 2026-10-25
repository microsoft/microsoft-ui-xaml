# Composition delta projections (P10 component, not P10 completion)

`CompositionTreeProcessor` is one of the five default owner-thread projections
and implements `IVerifiedBaselineProcessor`. Default registration covers ordinary
events, not verified-baseline routing or native collection.

## Observations and identity

- CompNodes and canonical CompositionVisuals remain distinct full `ObjectKey`s:
  target process instance, producer epoch, identity domain, and runtime ID.
  Neither display names nor addresses identify a parent, root, owner, or visual.
- Only typed `TreeEdgePayload` observations establish topology. Composition and
  unknown membership classifications remain distinct. Relationship changes end
  the old persisted row key before publishing the new one. Removal/disconnection
  is retained as membership, not destruction.
- A visual property without topology is retained and recorded in object history;
  it cannot manufacture a root, dispatcher, parent, or tree row. Later observed
  membership incorporates the properties known at that event time. Property units,
  original observation time, and explicit/unknown cause survive persistence.
- All correlations retain both endpoints, including endpoints without known
  births or topology. The index is many-to-many, never a single visual or owner
  field. Queries select either endpoint without reversing the canonical typed
  direction. Composition-owned correlation history records both endpoint directions.
  CorePeerAssociation history remains P09 ObjectStateProcessor's responsibility.
  ResourceUsageProcessor owns VisualResource and ResourceBacking indexes and
  histories; CompositionTreeProcessor only checks their endpoint liveness.
  Peer-to-visual relationships are not invented: existing typed kinds permit
  Core↔Peer and Core/CompNode↔Visual paths, not a direct Peer↔Visual edge.
- Explicit destruction tombstones observed memberships involving that endpoint.
  A detached child is not destroyed. Full identities cannot be reused after
  destruction or assigned multiple distinct birth times. Historical birth is
  never inferred from first observation; delta topology remains partial.

Every emitted revision uses the actual input source ordinal. Object history also
uses its exact QPC and ordinal. There are no synthetic source events or backdated
membership revisions.

## Time, uncertainty, and bounds

Retained, event-time-indexed facts allow late same-channel membership and property
updates when existing future snapshots remain valid. A root/child row-key switch
is tombstoned at the observed switch time. Cycles are checked at the incoming time
and affected later topology times. Contradictory same-QPC facts are rejected
conservatively rather than overwritten; equal repeated facts are idempotent.

Epoch-qualified loss beginning at its actual source timestamp invalidates carried
membership without disconnecting it. Unrelated producer epochs and explicitly
unrelated delivery domains do not invalidate it. Stale property values become
typed Missing values with their original units and observation timestamps; the
original history is unchanged. A fresh property does not refresh membership, and
a fresh edge does not refresh stale properties. Noninterval health invalidation
is applied at the next available source, not assigned an invented earlier time.

The default retained limit is 65,536 facts and 16 MiB of serialized fact accounting
plus a fixed 256-byte per-fact index allowance. Both limits are configurable
downward (entries up to 1,048,576; bytes at most 16 MiB). This is bounded accounting,
not an exact managed-heap measurement. Facts are never silently evicted. Limits
raise `ProcessingLimitException`, including oversized output records and exhausted
incremental record/byte budgets. The worker owns shared `OutputBudget` consumption;
the processor maintains a local emission allowance too, so direct enumeration is
bounded without double-consuming the worker budget.

Processor failures are terminal for that owner: discard its uncommitted tail and
instance. There is no rollback/retry/resume guarantee for a partially enumerated
or failed writer transaction.

## Explicit existing-schema and integration gaps

These are not represented as complete coverage or success-shaped state:

1. **Open correlation intervals are not representable.** `CorrelationEntry.Validity`
   must end at/before the current watermark, while the existing point query uses
   half-open interval filtering and latest-row dominance. Consequently this
   processor persists **instant-only** correlation observations, with an explicit
   quality reason, and unlink observations in both endpoint histories. A link is
   queryable at its observation QPC, not continuously until unlink. Temporal
   history is retained, but an open relationship state API needs a contract/query
   change outside this component.
2. **Backdated snapshot repair is unavailable.** The writer timestamps composition
   rows using the current transaction's real source. A late fact that would change
   a previously published future snapshot, a late new row key that escaped an
   already published destruction/loss, and retroactive composition loss affecting
   existing topology throw `InvalidDataException`. They do not publish a fabricated
   older source or silently preserve certainty. Safe late observations are covered
   by focused frozen-watermark tests.
3. **Terminal and retroactive health overlay needs query support.** Unlike the
   existing XAML query, the composition query does not overlay persisted loss or
   terminal ingestion diagnostics. Invalidation without another source cannot
   revise composition rows; the session health remains authoritative. No terminal
   composition certainty guarantee is claimed.
4. **Selected-subtree traversal is the existing query's behavior.** It currently
   does not stop composition traversal at detached/destroyed intermediate
   boundaries. Unselected per-membership projection state is tested; this component
   does not globally change descendant traversal.
5. **Resource correlation output has a single owner.** ResourceUsageProcessor
   emits VisualResource and ResourceBacking correlation indexes and both endpoint
   histories, for live events and verified baselines. CompositionTreeProcessor
   validates endpoint liveness but emits no duplicate rows for those kinds.
   Both processors receive the same ordinary source batch by default.
   ObjectStateProcessor owns lifetime/type and CorePeer history.
6. **Verified-baseline integration is explicit.** The coordinator calls
   `PublishBaseline(validatedEnd, ownedReconciledChunks, context)` **instead of**
   `Process` for that End. The real End is included once in the current writer
   transaction's sources; chunks are not repersisted or assigned synthetic
   sources. `ProjectionSupport.ValidatePublication` validates the handoff shape.
   Begin/cut/delivery continuity and concurrent-delta reconciliation remain the
   coordinator's responsibility. Raw Begin/chunks/End still never independently
   publish a baseline.

   Publication stages visual values before emitting topology, uses the actual
   End QPC/ordinal for all new observations, and enriches topology with available
   SnapshotObject names/classifications without claiming birth. CorePeer,
   Core/CompNode/Visual, and resource correlation observations use the same End.
   CorePeer baseline history remains owned by P09; unbound type dictionaries and
   Core/Peer metadata remain P09 responsibilities rather than inferred type links.
   Existing instant-only correlation limitations still apply; resource correlation
   output remains owned by ResourceUsageProcessor.

   Only complete boundary coverage **and** complete exact runtime TreeEdge
   delivery-domain coverage can retire absent memberships. Retirement is limited
   to the End's producer epoch and qualified root/dispatcher scope; partial
   captures cannot infer absence. Retirement is not object destruction and later
   property updates cannot resurrect retired memberships. Retained metadata,
   property facts, and retirement markers consume the same bounded state budget;
   all emitted records consume the incremental output allowance.

   End must be strictly later in QPC than retained topology/property facts.
   Same-tick/backdated publication requiring snapshot repair is rejected
   explicitly. Memberships are validated against retained topology while being
   applied, so a final-valid reparenting that temporarily forms a cycle can also
   be rejected conservatively; the component does not invent intermediate times.
   Failures are terminal for this processor instance, as for live deltas.

## Focused validation

From `tools\WinUIProfiler`, use a private artifacts directory:

```powershell
dotnet test WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj --no-restore --artifacts-path build\p10-composition-validation --filter FullyQualifiedName~CompositionTreeTests --logger 'console;verbosity=minimal'
```

The tests use real `SessionWriter`, query services, SQLite files, and read-only
reopen. They cover many-to-many/reverse links, typed properties, unknown endpoints,
distinct epochs/domains, membership versus destruction, safe late observations,
frozen watermarks, uncertainty, explicit invalid observations and unsupported
repairs, and retained/output limits. No Product build or full P10 coverage is
implied.
