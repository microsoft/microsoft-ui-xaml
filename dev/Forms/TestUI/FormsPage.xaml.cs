// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using System.Collections.ObjectModel;

#if !BUILD_WINDOWS
using Forms = Microsoft.UI.Xaml.Controls.Forms;
using ElementFactoryGetArgs = Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs;
using ElementFactoryRecycleArgs = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class FormsPage : TestPage
    {
        public FormsPage()
        {
            this.InitializeComponent();

            SuffixCombobox.Items.Add("Jr.");
            SuffixCombobox.Items.Add("Sr.");
            SuffixCombobox.Items.Add("Esq.");
        }
    }
}
