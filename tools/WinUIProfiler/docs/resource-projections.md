# Resource usage projection (P10)

`ResourceUsageProcessor` is one of the five default single-state-owner
`IEventProcessor` projections and implements `IVerifiedBaselineProcessor`.
It uses the existing resource, history and correlation DTOs, SQLite writer and
query service. Default registration handles ordinary events; verified-baseline
routing remains a separate integration gate.

## Persisted meanings

| Input | Projection |
|---|---|
| Resource Create | Exact-time `ResourceObservation(Create)` history and resource usage; creation is not inferred for any other operation. |
| BaselinePresent, Share, Resize, Rebind | Exact operation/cause history and the observed owner/device/backing/size descriptor. Changed owner/backing row keys tombstone the previous association in the same transaction. |
| OwnershipRelease, Close | Exact operation history, removal of the previous owned association and an unowned observation. These do **not** assert resource destruction, physical allocation release, or external COM destruction. |
| Destroy | Exact destruction history and tombstones for the previous/current resource association. This ends the classified resource, not all aliases of its backing. |
| Device Lost, Destroy | Missing-size, unknown-provenance revisions for associated resources; backing IDs and classified resource identities remain. No resource Destroy is synthesized. |
| Device OwnershipRelease | Recorded raw source only; this is not proof that backing bytes disappeared. |
| VisualResource, ResourceBacking correlation | Explicit, instant-only correlation plus typed histories at both endpoints. Unlink is a tombstone. No object birth, byte size, owner lifetime or open-ended correlation interval is inferred. |
| Loss / continuity health failure | The next source publishes missing-size revisions of carried resource descriptors. Later explicit observations may report size again, with partial continuity coverage. |

ResourceUsageProcessor is the sole emitter of VisualResource and ResourceBacking
correlation indexes and both endpoint histories, including verified-baseline
publication. CompositionTreeProcessor still validates endpoint liveness but does
not emit those rows, so co-registration cannot duplicate their projection keys.

Resource identity is the full `ObjectKey`: process instance, identity domain,
producer epoch and runtime ID. Neither addresses nor display names are identity.
Different epochs can reuse a numeric runtime ID without merging their histories.
The first non-Create observation explicitly reports **Unknown birth**. A
qualified identity cannot be created twice or mutated after destruction.
Conflicting authoritative resource kinds are rejected.

History retains exact event QPC, source ordinal, operation, classification,
reported owner, device, backing, size provenance and explicit attempt cause.
An absent cause is `Unknown`, not a fabricated causal attempt.
Source events remain the authority for fields the projection DTOs cannot express,
including pixel dimensions. Dimensions are never multiplied into physical bytes.
Object addresses, device API strings and device operations have no additional
resource-history fields in the existing DTOs.

## Bytes, sharing and quality

Each row is an **observed resource/backing descriptor**, not an independent
physical allocation. Shared backing IDs are preserved across resources and
owners. Physical backing identity must include the process instance, producer
epoch, device epoch and backing GUID. Never sum descriptor bytes per resource
or owner: aliases would double count. Identical backing observations can be
deduplicated; inconsistent observations at different times are not a proven
physical total. Conflicting simultaneous actual byte counts for the same backing
are rejected explicitly.

No process GPU committed/resident totals, unique-allocation totals, attribution
percentages, or synthetic metric samples are emitted. Those require independent
collector evidence and/or an allocation-level contract. Destruction of one
resource leaves other resource aliases intact; no physical backing-destruction
DTO exists here.

`ActualAllocation` stays Measured; `ApiDescriptionEstimate` stays Estimated.
Context/loss can degrade coverage without changing the measurement kind and
violating provenance validation. Unknown size is Missing/Bytes with unavailable
size quality, never zero. Counts above `long.MaxValue` cannot be represented
exactly in the integer DTO and are explicitly Missing/Unknown rather than
rounded to double or overflowed. Their original unsigned value remains in the
raw event. Sparse ownership releases do not silently carry an old byte
measurement or claim a known current backing: earlier identity/size evidence
remains in immutable history.

The resource DTO has one owner slot. This projection publishes the latest scalar
descriptor for a resource, including Share, rather than claiming an authoritative
many-owner retention graph. Additional explicit correlations remain separate;
XAML owners are not converted to invalid `VisualResource` edges. Multiple
resources sharing a backing retain their distinct owner slots and common backing
ID. A complete simultaneous many-owner resource projection needs coordinator/
contract semantics beyond this component.

## Event time and explicit limits

Usage revisions use the source's event QPC, not arrival order. Same-row-key
out-of-order facts (including destruction received before creation) remain
correct under the existing writer's event-time query dominance. Histories retain
their independent ingest order. Repeated identical facts at the same event time
are idempotent. Contradictory facts at the same time are rejected with
`InvalidDataException`, not last-writer-wins guesses.

The writer fixes a resource revision's QPC to its source QPC, and its row key
includes owner and backing. Therefore an arbitrarily late newly discovered
owner/backing association cannot always be repaired at a previously committed
successor or invalidation timestamp without fabricating a source. Such
cross-row-key retroactive transitions and late invalidations of newer state are
**explicitly rejected**, not published as apparently correct state. This is a
remaining ordering/repair-contract gap. A coordinator may supply appropriately
ordered events; the component does not buffer an unlimited reorder window.
Loss invalidations are published at the available source time, not backdated to
the beginning of a reported loss interval. Historical observations are not
rewritten.

All retained resource facts, device facts and resource-correlation facts share
an entry limit (default 65,536) and byte limit (default/cap 16 MiB).
The bounded dictionaries/ordered timelines and backing-conflict lookup retain
at most a constant number of index nodes per admitted fact. Serialized fact
bytes plus a fixed 256-byte per-fact index charge count against the byte limit.
Facts are retained to detect contradictions and identity reuse, including for
destroyed resources; long-running updates of one resource also consume the
limit. No historical registry grows without a bound and nothing is silently
evicted. Constructor overrides can lower limits for focused tests.

Limits throw `ProcessingLimitException`. Candidates are yielded one at a time
through `ProjectionSupport.Candidate`, checking the caller's remaining
`context.OutputBudget`; the owner must consume that same budget after each
yield. The processor does not collect unbounded output before yielding.
The existing worker stops/discards a failed batch and retains its previous
durable watermark. Do not reuse a processor after a thrown exception or
abandoned enumeration: in-memory mutation is not a transactional rollback/
resume protocol.

## Verified capture handoff

Ordinary Process calls never interpret CaptureChunk or raw End as a verified
resource baseline. The parent coordinator must validate Begin/chunks/End,
sequence cuts, scope, continuity and completeness. It can then call
`PublishBaseline(validatedEnd, chunks, context)`.
For a verified End, route that source through PublishBaseline **instead of**
also processing it through Process for this processor. This avoids competing
same-source invalidation/publication revisions. A pending health invalidation
is published for unstaged resources by the handoff itself; refreshed resources
receive their explicit presence observation with partial continuity.

The handoff applies `ProjectionSupport.ValidatePublication`, rejects duplicate
resource identities/resource-correlation keys before yielding, preserves End
coverage and converts staged resources to BaselinePresent, not Create. Every
candidate references only the validated End ordinal/QPC. The End **must be a
source in the same writer transaction**; the real writer rejects replaying an
old End in an unrelated transaction. Old chunks are not re-ingested. The
coordinator remains responsible for originally persisting the raw capture
sources. Publication does not clear prior continuity loss or claim a complete
physical allocation/lifetime baseline.

## Focused validation

Run from `tools\WinUIProfiler`:

```powershell
dotnet test WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj --artifacts-path build\p10-validation --no-restore --filter FullyQualifiedName~ResourceUsageTests --verbosity minimal
```

The private artifacts directory avoids other agents' ordinary bin/obj output.
If it has no assets file, restore the existing locked dependencies from the
repository's already populated package cache:

```powershell
dotnet restore WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj --artifacts-path build\p10-validation --locked-mode --source .packages --verbosity quiet
```

Tests use the actual `SessionWriter`, saved SQLite queries and read-only reopen;
they cover shared backing, unknown facts, epoch reuse, release versus destruction,
rebind/resize/reset, causal and out-of-order history, conflicts/duplicates,
continuity invalidation, entry/byte/output bounds, verified-End publication and
immutable watermark-pinned queries. No Product/native build or full test suite
is required for this managed component.
