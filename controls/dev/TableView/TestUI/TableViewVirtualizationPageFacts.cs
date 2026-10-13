// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared
{
    // Shared by the test app and the interaction tests so expectations use the page's own values.
    // Column widths mirror the XAML by hand.
    internal static class TableViewVirtualizationPageFacts
    {
        internal const string NavigateButton = "navigateToVirtualization";
        internal const string LargeTable = "LargeEditableTable";
        internal const string RemoveFirstItemButton = "RemoveFirstItemButton";

        // Read-only TextBox describing LargeEditableTable, as "key=value" fields joined by ';':
        //   editing=<True|False>    LargeEditableTable.IsEditing
        //   beginning=<n>           BeginningEdit count
        //   ending=<entries|none>   one "<EditAction>/<item Id>" per CellEditEnding, joined by ','
        //   first=<name>            Name of the item that was at source index 0 when the page loaded,
        //                           still tracked after it is removed
        //   modified=<ids|none>     ascending Ids of every item ever created whose values differ from the seed
        //   editors=<entries|none>  one "<item Id>/<editor text>" per TextBox inside a realized row, joined by ','
        //   visible=<a-b|none>      first and last item Id of the rows intersecting the body viewport
        //   offset=<v>              PART_BodyScroller.VerticalOffset
        internal const string Readout = "VirtualizationReadout";

        internal const string EditingField = "editing";
        internal const string BeginningEditField = "beginning";
        internal const string EditEndingField = "ending";
        internal const string FirstItemField = "first";
        internal const string ModifiedField = "modified";
        internal const string EditorsField = "editors";
        internal const string VisibleField = "visible";
        internal const string OffsetField = "offset";
        internal const string NoneValue = "none";

        internal static readonly string[] Columns = { "Name", "Quantity", "Code" };
        internal static readonly int[] ColumnWidths = { 200, 120, 160 };

        internal const int NameColumn = 0;
        internal const int QuantityColumn = 1;
        internal const int CodeColumn = 2;

        internal static int ColumnCentreX(int column)
        {
            int left = 0;
            for (int i = 0; i < column; i++)
            {
                left += ColumnWidths[i];
            }
            return left + (ColumnWidths[column] / 2);
        }

        internal const int ItemCount = 2000;

        internal static string ItemName(int id) => "Item " + id;
        internal static int ItemQuantity(int id) => id % 100;
        internal static string ItemCode(int id) => "C" + id.ToString("D4", global::System.Globalization.CultureInfo.InvariantCulture);
    }
}
