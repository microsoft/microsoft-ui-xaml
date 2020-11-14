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

using PipsControl = Microsoft.UI.Xaml.Controls.PipsControl;
using PipsControlSelectedIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.PipsControlSelectedIndexChangedEventArgs;
using Windows.UI.Xaml.Input;
using System.Collections.ObjectModel;
using System.Collections.Generic;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "PipsControl")]
    public sealed partial class PipsControlPage : TestPage
    {
        public List<string> Pictures { get; } = new List<string>() 
        {
            "Assets/ingredient1.png",
            "Assets/ingredient2.png",
            "Assets/ingredient3.png",
            "Assets/ingredient4.png",
            "Assets/ingredient5.png",
            "Assets/ingredient6.png",
            "Assets/ingredient7.png",
            "Assets/ingredient8.png"
        };

        public PipsControlPage()
        {
            this.InitializeComponent();
        }
    }

}
