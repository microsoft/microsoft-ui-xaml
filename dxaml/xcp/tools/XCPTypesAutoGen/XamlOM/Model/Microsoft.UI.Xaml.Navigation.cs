// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Navigation
{
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "548ad04e-f6f3-4d0e-a146-7b641c0b970a")]
    public sealed class PageStackEntry
     : Microsoft.UI.Xaml.DependencyObject
    {
        [FieldBacked]
        public Windows.UI.Xaml.Interop.TypeName SourcePageType
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Parameter
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo NavigationTransitionInfo
        {
            get;
            internal set;
        }

        internal PageStackEntry() { }

        public PageStackEntry(Windows.UI.Xaml.Interop.TypeName sourcePageType, [Optional] Windows.Foundation.Object parameter, [Optional] Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo navigationTransitionInfo) { }
    }


    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "f143da9c-da24-425c-b251-08106656211f")]
    public sealed class NavigatingCancelEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Navigation.NavigationMode NavigationMode
        {
            get;
            internal set;
        }

        public Windows.UI.Xaml.Interop.TypeName SourcePageType
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Parameter
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo NavigationTransitionInfo
        {
            get;
            internal set;
        }

        internal NavigatingCancelEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "6b01873f-18c1-4fa5-8dbf-70f91388e193")]
    public sealed class NavigationFailedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.HRESULT Exception
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public Windows.UI.Xaml.Interop.TypeName SourcePageType
        {
            get;
            internal set;
        }

        internal NavigationFailedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CNavigationEventArgs")]
    [Guids(ClassGuid = "4d748b64-3e34-4bd4-8f95-90a97c9422e6")]
    public sealed class NavigationEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Content
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Parameter
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo NavigationTransitionInfo
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.UI.Xaml.Interop.TypeName SourcePageType
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Navigation.NavigationMode NavigationMode
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strUrl")]
        public Windows.Foundation.Uri Uri
        {
            get;
            set;
        }

        internal NavigationEventArgs() { }
    }

    [TypeTable(IsExcludedFromCore = true)]
    public enum NavigationMode
    {
        [NativeValueName("NavigationModeNew")]
        New = 0,
        [NativeValueName("NavigationModeBack")]
        Back = 1,
        [NativeValueName("NavigationModeForward")]
        Forward = 2,
        [NativeValueName("NavigationModeRefresh")]
        Refresh = 3,
    }

    [TypeTable(IsExcludedFromCore = true)]
    public enum NavigationCacheMode
    {
        [NativeValueName("NavigationCacheModeDisabled")]
        Disabled = 0,
        [NativeValueName("NavigationCacheModeRequired")]
        Required = 1,
        [NativeValueName("NavigationCacheModeEnabled")]
        Enabled = 2,
    }

    public delegate void NavigatedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Navigation.NavigationEventArgs e);

    public delegate void NavigatingCancelEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Navigation.NavigatingCancelEventArgs e);

    public delegate void NavigationFailedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Navigation.NavigationFailedEventArgs e);

    public delegate void NavigationStoppedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Navigation.NavigationEventArgs e);

    [FrameworkTypePattern]
    [Guids(ClassGuid = "50ac0c23-ca18-4f64-a9fd-d94db1b73814")]
    public class FrameNavigationOptions
    {
        public FrameNavigationOptions() { }
        
        public Windows.Foundation.Boolean IsNavigationStackEnabled
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo TransitionInfoOverride
        {
            get;
            set;
        }
    }
}
