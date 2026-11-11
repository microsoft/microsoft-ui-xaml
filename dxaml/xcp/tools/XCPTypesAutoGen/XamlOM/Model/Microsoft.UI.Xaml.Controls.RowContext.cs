// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    // Experimental. Neighbor information that ListView maintains for each realized ListViewItem.
    // See docs\api-specs\ListView\RowContext-spec.md.

    [Comment("Describes the relationship between a row and the row next to it in the displayed list.")]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.Experimental)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum NeighborRelation
    {
        None = 0,
        SameGroup = 1,
        OtherGroup = 2,
    }

    [Comment("Read-only attached properties that ListView sets on its ListViewItem containers to describe neighboring rows.")]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.Experimental)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "95d3c221-99f7-413d-942f-170aaeb26e14")]
    public static class RowContext
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.Controls.ListViewItem))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.Controls.NeighborRelation AttachedPreviousRelation
        {
            get;
            internal set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.Controls.ListViewItem))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.Controls.NeighborRelation AttachedNextRelation
        {
            get;
            internal set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.Controls.ListViewItem))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public static Windows.Foundation.Boolean AttachedIsPreviousSelected
        {
            get;
            internal set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.Controls.ListViewItem))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public static Windows.Foundation.Boolean AttachedIsNextSelected
        {
            get;
            internal set;
        }
    }
}
