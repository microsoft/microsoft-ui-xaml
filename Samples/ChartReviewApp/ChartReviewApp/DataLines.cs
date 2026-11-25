// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;

namespace ChartsSample
{
    // Shows data one line per TextBlock, so a screen reader reads each line on its own. In a single
    // multi-line TextBlock, Narrator reads a value together with the next line's label, for example
    // "8, Feb" as "February 8th". Lines are updated in place, so a live list keeps the reader's place.
    public sealed partial class DataLines : StackPanel
    {
        public void SetLines(IReadOnlyList<string> lines)
        {
            for (int i = 0; i < lines.Count; i++)
            {
                if (i < Children.Count)
                {
                    var line = (TextBlock)Children[i];
                    if (line.Text != lines[i]) line.Text = lines[i];
                }
                else
                {
                    Children.Add(new TextBlock { Text = lines[i], IsTextSelectionEnabled = true, TextWrapping = Microsoft.UI.Xaml.TextWrapping.Wrap });
                }
            }
            while (Children.Count > lines.Count)
            {
                Children.RemoveAt(Children.Count - 1);
            }
        }
    }
}
