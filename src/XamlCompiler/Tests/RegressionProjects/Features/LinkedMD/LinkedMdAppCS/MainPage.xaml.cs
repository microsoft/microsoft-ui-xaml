// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace AppCS
{
    public sealed partial class MainPage : Page
    {
        static IXamlType EnsureTypeExists(String typeName)
        {
            var ixmp = Application.Current as IXamlMetadataProvider;
            var t = ixmp.GetXamlType(typeName);
            if (t == null)
            {
                throw new ArgumentException(typeName);
            }
            return t;
        }

        static IXamlMember EnsureMemberExists(IXamlType t, String memberName)
        {
            var m = t.GetMember(memberName);
            if (m == null)
            {
                throw new ArgumentException(memberName);
            }
            return m;
        }

        public MainPage()
        {
            this.InitializeComponent();
            IXamlType t;

            t = EnsureTypeExists("AppCS.MainPage");

            // CS
            t = EnsureTypeExists("ControlsCS.A");
            EnsureMemberExists(t, "StringPropertyOnA");
            EnsureMemberExists(t, "BPropertyOnA");
            t = EnsureTypeExists("ControlsCS.B");
            EnsureMemberExists(t, "StringPropertyOnB");
            t = EnsureTypeExists("SubControlsCS.S");
            EnsureMemberExists(t, "StringPropertyOnS");
            EnsureMemberExists(t, "TPropertyOnS");
            t = EnsureTypeExists("SubControlsCS.T");
            EnsureMemberExists(t, "StringPropertyOnT");

            // CX
            t = EnsureTypeExists("ControlsCX.A");
            EnsureMemberExists(t, "StringPropertyOnA");
            EnsureMemberExists(t, "BPropertyOnA");
            t = EnsureTypeExists("ControlsCX.B");
            EnsureMemberExists(t, "StringPropertyOnB");
            t = EnsureTypeExists("SubControlsCX.S");
            EnsureMemberExists(t, "StringPropertyOnS");
            EnsureMemberExists(t, "TPropertyOnS");
            t = EnsureTypeExists("SubControlsCX.T");
            EnsureMemberExists(t, "StringPropertyOnT");

            // C++
            t = EnsureTypeExists("LinkedMDControlsCppWinRT.A");
            EnsureMemberExists(t, "StringPropertyOnA");
            EnsureMemberExists(t, "BPropertyOnA");
            t = EnsureTypeExists("LinkedMDControlsCppWinRT.B");
            EnsureMemberExists(t, "StringPropertyOnB");
            t = EnsureTypeExists("LinkedMDSubControlsCppWinRT.S");
            EnsureMemberExists(t, "StringPropertyOnS");
            EnsureMemberExists(t, "TPropertyOnS");
            t = EnsureTypeExists("LinkedMDSubControlsCppWinRT.T");
            EnsureMemberExists(t, "StringPropertyOnT");
        }
    }
}
