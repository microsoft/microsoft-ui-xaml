// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

public sealed partial class FileExplorerPage
{
    protected override void RefreshReadouts()
    {
        FolderText.Text = _currentDir.Length > 0 ? _currentDir : "(reading)";
        ItemsText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} items ({1:N0} folders)", Entries.Count, Entries.Count(entry => entry.IsFolder));
        SelectedText.Text = FileTable.SelectedItem switch
        {
            FileSystemEntry { IsFolder: true } folder => string.Format(CultureInfo.CurrentCulture, "{0} (folder)", folder.Name),
            FileSystemEntry file => string.Format(CultureInfo.CurrentCulture, "{0} ({1})", file.Name, file.SizeDisplay),
            _ => "(none)",
        };
        Status.Rows = SampleShaping.RowCountText(VisibleCount());
    }
}
