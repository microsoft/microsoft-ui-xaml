// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Controls
{
    [Platform(typeof(PrivateApiContract), 1)]
    public interface IMenu
    {
        void Close();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        IMenu ParentMenu
        {
            get;
            set;
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IMenuPresenter
    {
        void CloseSubMenu();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        IMenu OwningMenu
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        ISubMenuOwner Owner
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        IMenuPresenter SubPresenter
        {
            get;
            set;
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ISubMenuOwner
    {
        void PrepareSubMenu();

        void OpenSubMenu(Windows.Foundation.Point position);

        void PositionSubMenu(Windows.Foundation.Point position);

        void ClosePeerSubMenus();

        void CloseSubMenu();

        void CloseSubMenuTree();

        void DelayCloseSubMenu();

        void CancelCloseSubMenu();

        void RaiseAutomationPeerExpandCollapse(Windows.Foundation.Boolean isOpen);

        void SetSubMenuDirection(Windows.Foundation.Boolean isSubMenuDirectionUp);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsSubMenuOpen
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsSubMenuPositionedAbsolutely
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        ISubMenuOwner ParentOwner
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "5c2b5964-9c83-47b8-bfcb-3c12b39181b9")]
    public class MenuFlyoutItemBase
     : Microsoft.UI.Xaml.Controls.Control
    {
        internal MenuFlyoutItemBase() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "1873178e-9f40-4cc7-a2a9-4aba5bfffefb")]
    public class MenuFlyoutSeparator
     : Microsoft.UI.Xaml.Controls.MenuFlyoutItemBase
    {
        public MenuFlyoutSeparator() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [ContentProperty("Text")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "d39d96d6-cc5d-4df8-85f5-b4f353f1f6e8")]
    public class MenuFlyoutItem
     : Microsoft.UI.Xaml.Controls.MenuFlyoutItemBase
    {
        public MenuFlyoutItem() { }

        public Windows.Foundation.String Text
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Input.ICommand Command
        {
            get;
            set;
        }

        public Windows.Foundation.Object CommandParameter
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.IconElement Icon
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.String KeyboardAcceleratorTextOverride
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.MenuFlyoutItemTemplateSettings TemplateSettings
        {
            get;
            private set;
        }

        internal Windows.Foundation.Boolean PreventDismissOnPointer
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.RoutedEventHandler Click;
    }

    [CodeGen(partial: true)]
    [ContentProperty("Items")]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CMenuFlyoutSubItem")]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ISubMenuOwner))]
    [Guids(ClassGuid = "cb6688cb-2d2c-4270-9a0b-32cf8d447c9c")]
    public sealed class MenuFlyoutSubItem
     : Microsoft.UI.Xaml.Controls.MenuFlyoutItemBase
    {
        public MenuFlyoutSubItem() { }

        [CollectionType(CollectionKind.Vector)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.MenuFlyoutItemBaseCollection Items
        {
            get;
            internal set;
        }

        public Windows.Foundation.String Text
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.IconElement Icon
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "ea2a987b-d49d-4e63-bba3-482c530741cf")]
    public class ToggleMenuFlyoutItem
     : Microsoft.UI.Xaml.Controls.MenuFlyoutItem
    {
        public ToggleMenuFlyoutItem() { }

        public Windows.Foundation.Boolean IsChecked
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [ClassFlags(IsHiddenFromIdl = true)]
    [NativeName("CMenuFlyoutItemBaseCollection")]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "dc3758f8-50c1-4dae-a8aa-590fe0c5f059")]
    public sealed class MenuFlyoutItemBaseCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<MenuFlyoutItemBase>
    {
        [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))]
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Controls.MenuFlyoutItemBase ContentProperty
        {
            get;
            set;
        }

        internal MenuFlyoutItemBaseCollection() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CMenuFlyout")]
    [ContentProperty("Items")]
    [FrameworkTypePattern]
    [InstanceCountTelemetry]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IMenu))]
    [Guids(ClassGuid = "8eec1227-1eed-454b-baa3-6402349d57dd")]
    public class MenuFlyout
     : Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase
    {
        public MenuFlyout() { }

        [CollectionType(CollectionKind.Vector)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.MenuFlyoutItemBaseCollection Items
        {
            get;
            internal set;
        }

        public Microsoft.UI.Xaml.Style MenuFlyoutPresenterStyle
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void ShowAt([Optional] Microsoft.UI.Xaml.UIElement targetElement, Windows.Foundation.Point point)
        {
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CMenuFlyoutPresenter")]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IMenuPresenter))]
    [Guids(ClassGuid = "a21c5006-9b67-4df9-829a-0ed60a5ad01e")]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.LatestVersion)]
    [Velocity(Feature = "Feature_ExperimentalApi")]
    public class MenuFlyoutPresenter
     : Microsoft.UI.Xaml.Controls.ItemsControl
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.MenuFlyoutPresenterTemplateSettings TemplateSettings
        {
            get;
            private set;
        }

        public Windows.Foundation.Boolean IsDefaultShadowEnabled { get; set; }

        public MenuFlyoutPresenter() { }

        [RequiresMultipleAssociationCheck]
        [VelocityFeature("Feature_ExperimentalApi")]
        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop { get; set; }
    }
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "8cc8a5c3-c979-4830-aa5f-3d5df0c623b3")]
    public class MenuFlyoutPresenterAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ItemsControlAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public MenuFlyoutPresenterAutomationPeer(Microsoft.UI.Xaml.Controls.MenuFlyoutPresenter owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "027eaa56-c9ea-4d23-aeed-f980cbc25173")]
    public class MenuFlyoutItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public MenuFlyoutItemAutomationPeer(Microsoft.UI.Xaml.Controls.MenuFlyoutItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider))]
    [Guids(ClassGuid = "802e2395-2a2c-49bf-b771-9b71df98798d")]
    internal class MenuFlyoutSubItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public MenuFlyoutSubItemAutomationPeer(Microsoft.UI.Xaml.Controls.MenuFlyoutSubItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IToggleProvider))]
    [Guids(ClassGuid = "1b97cfcf-713b-4e16-96f8-fa4020a8d468")]
    public class ToggleMenuFlyoutItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ToggleMenuFlyoutItemAutomationPeer(Microsoft.UI.Xaml.Controls.ToggleMenuFlyoutItem owner)
            : base(owner) { }
    }
}
