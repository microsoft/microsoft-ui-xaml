// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;

namespace MUXControlsAdhocApp
{
    public sealed partial class CommandBarFlyoutPage : Page
    {
        public CommandBarFlyoutPage()
        {
            InitializeComponent();

            CutButton.Command = new StandardUICommand(StandardUICommandKind.Cut);
            CopyButton.Command = new StandardUICommand(StandardUICommandKind.Copy);
            PasteButton.Command = new StandardUICommand(StandardUICommandKind.Paste);
            UndoButton.Command = new StandardUICommand(StandardUICommandKind.Undo);
            RedoButton.Command = new StandardUICommand(StandardUICommandKind.Redo);
            SelectAllButton.Command = new StandardUICommand(StandardUICommandKind.SelectAll) { IconSource = null };
        }
    }
}
