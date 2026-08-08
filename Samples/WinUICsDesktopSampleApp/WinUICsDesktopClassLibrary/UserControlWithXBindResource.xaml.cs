// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

namespace WinUICsDesktopClassLibrary
{
	// This is a minimal repro for verifying that x:Bind can be used in a ResourceDictionary resource
	// without triggering a NullReferenceException
    public sealed partial class UserControlWithXBindResource : UserControl
    {
        public UserControlWithXBindResource()
        {
            this.InitializeComponent();
        }
    }
}
