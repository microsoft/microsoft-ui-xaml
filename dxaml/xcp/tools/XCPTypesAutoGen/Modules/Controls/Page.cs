// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "b0e467bb-e8ee-4cf2-af8a-106295e5efef")]
    public class Page
     : Microsoft.UI.Xaml.Controls.UserControl
    {
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.Frame Frame { get; internal set; }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.Navigation.NavigationCacheMode NavigationCacheMode { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.AppBar TopAppBar { get; set; }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.AppBar BottomAppBar { get; set; }

        public Page() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnNavigatedFrom(Microsoft.UI.Xaml.Navigation.NavigationEventArgs e) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnNavigatedTo(Microsoft.UI.Xaml.Navigation.NavigationEventArgs e) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnNavigatingFrom(Microsoft.UI.Xaml.Navigation.NavigatingCancelEventArgs e) { }
    }
}
