// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace TableViewSampleApp.Pages;

/// <summary>
/// Placeholder for hierarchical rows, which are not available in this release.
/// The page deliberately has no behaviour: it describes the axis in prose and shows
/// the canonical Shaping section with every option except Flat disabled, rather than
/// shipping controls that no-op or naming an API that does not exist.
/// </summary>
public sealed partial class HierarchyPage : SamplePageBase
{
    public HierarchyPage()
    {
        InitializeComponent();
        InitializeSample(Status);
    }
}
