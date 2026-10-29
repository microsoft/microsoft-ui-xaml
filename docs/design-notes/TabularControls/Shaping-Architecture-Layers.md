# Shaping architecture: layers

A one-page map of the `TableViewSource2` shaping pipeline (`controls/dev/TableViewSource2`), grouped
by folder. The full design and the decisions behind it are in
[Shaping-Architecture-Redesign.md](Shaping-Architecture-Redesign.md).

Legend: solid arrows are ownership or reads, thick arrows are events, dotted arrows are local or
helper relationships.

<details>
<summary><b>Layer diagram</b></summary>

```mermaid
flowchart TB
    App(["App / TableView"])
    Owner(["IShapingOwner<br/>(TableView)"])

    subgraph L1["WinRT surface"]
        TVS["TableViewSource2<br/>FilterBy / SortBy / GroupBy / Hierarchy / DeferRefresh"]
        DRH["DeferRefreshHandle"]
    end

    subgraph L2["Shaping/ : coordinator"]
        SM["ShapingManager<br/>desired ShapeSpec, defer, live drain, owner hold<br/>BeginCommit / EndCommit, m_commit"]
    end

    subgraph L3["View/ : read-only adapters"]
        SCV["ShapedCollectionView<br/>ItemsSourceView: GetAt / IndexOf / Count"]
        SRM["ShapedRowMetadata<br/>level, kind, expanded, reveal"]
        GHR["GroupHeaderRow<br/>row item for a group"]
    end

    subgraph L4["Spec/ : what to shape"]
        SS["ShapeSpec<br/>FilterSpec, SortSpec, GroupSpec"]
        SD["ShapeDelegates"]
        PE["PathEvaluator"]
    end

    subgraph L5["Engine/ : shaped tree"]
        ENG["ShapingEngine<br/>compute then noexcept write<br/>Apply / Reapply / ApplyItemsChanged"]
        STG["StagedEdit / StagedReapply<br/>per-call compute buffer"]
        TREE["ShapedTree<br/>rows by position, counts, cursor, expand<br/>Link / Unlink, BeginReset / EndReset"]
        GN["GroupNode<br/>Key, Children, ItemCount<br/>owned by the engine's GroupMap"]
        GE["GroupExpansion<br/>group Identity to expanded"]
        FW["FenwickTree<br/>ChildRows per sibling list"]
    end

    subgraph L6["Source/ : what the app gave us"]
        ST["SourceTable<br/>records, lookups, freeze, SourceChange events"]
        SR["SourceRecord<br/>Item, Parent, Children<br/>+ ShapeState inline"]
        SH["SourceHierarchy<br/>ParentKey or ChildCollections"]
        CA["CollectionAccessor2<br/>IVector / IBindableVector / IIterable"]
    end

    App -->|"verbs"| TVS
    TVS -->|"owns"| SM
    TVS -.-> DRH
    SM -->|"owns"| ENG
    SM -->|"owns"| SD
    SM -->|"owns"| PE
    SM -->|"holds desired"| SS
    SM -->|"shared_ptr"| ST
    SM -->|"creates"| SCV
    SM -->|"creates"| SRM
    SCV -->|"reads"| TREE
    SRM -->|"reads"| TREE
    SCV -.->|"group rows"| GHR
    ENG -->|"owns"| TREE
    ENG -.->|"local"| STG
    ENG -->|"reads keys via"| SD
    ENG -->|"reads keys via"| PE
    ENG -->|"reads"| ST
    TREE -->|"owns root"| GN
    TREE -->|"owns"| GE
    TREE -->|"counts in"| FW
    TREE -->|"writes ShapeState"| SR
    GN -->|"Children"| SR
    ST -->|"owns"| SR
    ST -->|"owns"| SH
    ST -->|"reads"| CA
    SH -->|"links Parent / Children"| SR

    ST ==>|"SourceChanged / ItemChanged"| SM
    TREE ==>|"IShapedRowsListener<br/>OnRowsInserted / Removed / Reset"| SM
    SM ==>|"NotifyAdded / Removed / Reset"| SCV
    SM ==>|"OnResetting / OnShapingApplied"| Owner
    SCV ==>|"CollectionChanged"| App
```

</details>

## Layers

- **Source/** mirrors the app's data: records and their `Parent` / `Children` links. It knows
  nothing about shaping. `SourceRecord` carries `ShapeState` inline, but Source/ never writes it.
- **Spec/** describes the desired shape (`ShapeSpec`) and evaluates keys (`ShapeDelegates`,
  `PathEvaluator`).
- **Engine/** computes filter, sort and grouping. `ShapingEngine` computes into a local
  `StagedEdit` / `StagedReapply` buffer, then writes in a `noexcept` phase. `ShapedTree` is the
  only owner of the rows: it holds the group root, the visible-row counts (`FenwickTree` per
  sibling list), the cursor, the expansion verbs and `GroupExpansion` (pruned at each
  `EndReset`), and it writes the tree-owned `ShapeState` fields.
- **Shaping/** coordinates one commit. `ShapingManager` listens to `SourceTable` and to the tree
  (`IShapedRowsListener`) and turns tree events into view notifications and owner callbacks.
- **View/** holds read-only adapters over `ShapedTree`; no rows are stored there.
