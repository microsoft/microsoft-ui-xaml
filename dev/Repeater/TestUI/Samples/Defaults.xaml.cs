// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Linq;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class Defaults : Page
    {
        public List<string> Data { get; set; }

        public Defaults()
        {
            Data = Enumerable.Range(0, 10000).Select(x => x.ToString()).ToList();
            this.InitializeComponent();
        }
    }
}
