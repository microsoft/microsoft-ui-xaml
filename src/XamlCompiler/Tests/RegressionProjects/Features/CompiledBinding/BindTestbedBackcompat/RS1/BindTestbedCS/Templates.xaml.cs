// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml.Data;

namespace BindTestbed
{
    [Bindable]
    internal partial class Templates
    {
        public Templates()
        {
            InitializeComponent();
            DetectLeaksPage.TrackObject(this);
        }
    }
}
