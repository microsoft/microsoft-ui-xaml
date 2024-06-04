// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "d877e5a7-1390-46b6-ab82-823ef1ae959e")]
    public class PivotHeaderItem : Controls.ContentControl
    {
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.IValueConverter))]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "7360112a-c84c-497e-a502-5647c6cf70f7")]
    public sealed class JumpListItemForegroundConverter
        : DependencyObject
    {
        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Microsoft.UI.Xaml.Media.Brush Enabled { get; set; }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Microsoft.UI.Xaml.Media.Brush Disabled { get; set; }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.IValueConverter))]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "5aae91c3-be79-4eea-9841-446096b8658c")]
    public sealed class JumpListItemBackgroundConverter
        : DependencyObject
    {
        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Microsoft.UI.Xaml.Media.Brush Enabled { get; set; }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Microsoft.UI.Xaml.Media.Brush Disabled { get; set; }
    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.OnPropertyChangeCallback)]
    [Guids(ClassGuid = "93790753-7ded-46d9-bf50-eced4011793d")]
    public sealed class LoopingSelector : Controls.Control
    {
        internal LoopingSelector() { }

        public Windows.Foundation.Boolean ShouldLoop
        {
            get;
            set;
        }

        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Controls.ItemCollection Items
        {
            get;
            set;
        }

        public Windows.Foundation.Int32 SelectedIndex
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public Windows.Foundation.Object SelectedItem
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public Windows.Foundation.Int32 ItemWidth
        {
            get;
            set;
        }

        public Windows.Foundation.Int32 ItemHeight
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public DataTemplate ItemTemplate
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.SelectionChangedEventHandler SelectionChanged;

    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "2c512cfe-9f07-418d-a175-db1c13aa1920")]
    public sealed class LoopingSelectorItem : Controls.ContentControl
    {
        internal LoopingSelectorItem() { }
    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollSnapPointsInfo))]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "f2120228-0659-4f89-9357-91ac7daa0dff")]
    public sealed class LoopingSelectorPanel : Controls.Canvas
    {
        internal LoopingSelectorPanel() { }
    }

    [DXamlIdlGroup("Phone")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollSnapPointsInfo))]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "b1b5d90d-aba6-436b-bf32-2f9f7f780429")]
    public sealed class PivotPanel : Controls.Panel
    {
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "6b1f247b-5cc4-480c-bf46-182dcd94c8da")]
    public sealed class PivotHeaderPanel : Controls.Canvas
    {
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "da33a6e2-6d66-4f5d-85a6-c001c70358ba")]
    public abstract class PickerFlyoutBase : Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.String AttachedTitle
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnConfirmed()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected abstract Windows.Foundation.Boolean ShouldShowConfirmationButtons();
    }
}
