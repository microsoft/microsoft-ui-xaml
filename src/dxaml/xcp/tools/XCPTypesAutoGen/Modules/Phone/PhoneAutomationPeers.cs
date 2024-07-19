// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollProvider))]
    [Guids(ClassGuid = "ae6fc00a-1cb2-4065-a084-1620efdc6d3c")]
    public sealed class PivotAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.ItemsControlAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public PivotAutomationPeer(Microsoft.UI.Xaml.Controls.Pivot owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "6043b0c0-a9b3-4887-a766-b351b7be6247")]
    public sealed class PivotItemAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public PivotItemAutomationPeer(Microsoft.UI.Xaml.Controls.PivotItem owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IVirtualizedItemProvider))]
    [Guids(ClassGuid = "aec050fd-a609-44fc-bce9-7f40989e389d")]
    public sealed class PivotItemDataAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.ItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public PivotItemDataAutomationPeer(
            Windows.Foundation.Object item,
            Microsoft.UI.Xaml.Automation.Peers.PivotAutomationPeer parent)
            : base() { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "1a49e873-27dd-46af-9c4d-eff7b6139466")]
    public sealed class DatePickerFlyoutPresenterAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        internal DatePickerFlyoutPresenterAutomationPeer(Microsoft.UI.Xaml.Controls.DatePickerFlyoutPresenter owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "9a504245-a3c2-4843-b741-fae0eb6e559d")]
    public sealed class TimePickerFlyoutPresenterAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        internal TimePickerFlyoutPresenterAutomationPeer(Microsoft.UI.Xaml.Controls.TimePickerFlyoutPresenter owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "d66088ca-4ea1-42bd-8e10-22ef3ebc02ff")]
    public sealed class ListPickerFlyoutPresenterAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        internal ListPickerFlyoutPresenterAutomationPeer(Microsoft.UI.Xaml.Controls.ListPickerFlyoutPresenter owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "039a5a02-fd9d-4005-b188-6a42cb6bb4b3")]
    public sealed class PickerFlyoutPresenterAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        internal PickerFlyoutPresenterAutomationPeer(Microsoft.UI.Xaml.Controls.PickerFlyoutPresenter owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IItemContainerProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollProvider))]
    [Guids(ClassGuid = "a596e627-f803-4ec6-a4d9-54911eceb78b")]
    public sealed class LoopingSelectorAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        internal LoopingSelectorAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.LoopingSelector owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ISelectionItemProvider))]
    [Guids(ClassGuid = "702e7519-6955-4d9b-a817-6facec9a0523")]
    public sealed class LoopingSelectorItemAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        internal LoopingSelectorItemAutomationPeer(Microsoft.UI.Xaml.Controls.Primitives.LoopingSelectorItem owner)
            : base(owner) { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IVirtualizedItemProvider))]
    [Guids(ClassGuid = "76cb8941-ba86-41c7-b5da-8a1df5b38b3e")]
    public sealed class LoopingSelectorItemDataAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.AutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        internal LoopingSelectorItemDataAutomationPeer(Windows.Foundation.Object item,
            Microsoft.UI.Xaml.Automation.Peers.LoopingSelectorAutomationPeer parent)
            : base() { }
    }

}
