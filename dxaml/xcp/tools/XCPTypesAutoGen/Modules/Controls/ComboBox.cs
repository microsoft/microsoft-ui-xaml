// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Platform("Feature_InputValidation", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl), Velocity = "Feature_InputValidation")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl2), Velocity = "Feature_InputValidation")]
    [Guids(ClassGuid = "f89c3331-c1f8-435a-909f-584cfe4947f5")]
    [TemplatePart("ItemsPresenter", typeof(Microsoft.UI.Xaml.Controls.ItemsPresenter))]
    [TemplatePart("ItemsPresenterTranslateTransform", typeof(Microsoft.UI.Xaml.Media.TranslateTransform))]
    [TemplatePart("ItemsPresenterHost", typeof(Microsoft.UI.Xaml.Controls.Canvas))]
    [TemplatePart("FlyoutButton", typeof(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase))]
    [TemplatePart("Background", typeof(Microsoft.UI.Xaml.Controls.Border))]
    [TemplatePart("ContentPresenter", typeof(Microsoft.UI.Xaml.Controls.ContentPresenter))]
    [TemplatePart("Popup", typeof(Microsoft.UI.Xaml.Controls.Primitives.Popup))]
    [TemplatePart("EditableText", typeof(Microsoft.UI.Xaml.Controls.TextBox))]
    [TemplatePart("DropDownOverlay", typeof(Microsoft.UI.Xaml.Controls.Border))]
    [InputProperty("Text")]
    public class ComboBox : Microsoft.UI.Xaml.Controls.Primitives.Selector
    {
        #region Properties

        // Version 1
        public Windows.Foundation.Boolean IsDropDownOpen { get; set; }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Boolean IsEditable { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.DependencyProperty IsEditableProperty { get; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Boolean IsSelectionBoxHighlighted { get; private set; }

        public Windows.Foundation.Double MaxDropDownHeight { get; set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Object SelectionBoxItem { get; private set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.DataTemplate SelectionBoxItemTemplate { get; private set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.ComboBoxTemplateSettings TemplateSettings { get; private set; }

        public Windows.Foundation.Object Header { get; set; }

        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; }

        public Windows.Foundation.String PlaceholderText { get; set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        public Windows.Foundation.Boolean IsTextSearchEnabled { get; set; }

        public Microsoft.UI.Xaml.Controls.ComboBoxSelectionChangedTrigger SelectionChangedTrigger { get; set; }

        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PlaceholderForeground { get; set; }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueString)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.String Text { get; set; }

        public Microsoft.UI.Xaml.Style TextBoxStyle { get; set; }

        public object Description { get; set; }

        // Velocity
        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement { get; set; }

        #endregion

        #region Events

        // Version 1
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler DropDownClosed;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler DropDownOpened;

        [EventFlags(UseEventManager = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Windows.Foundation.TypedEventHandler<ComboBox, ComboBoxTextSubmittedEventArgs> TextSubmitted;

        #endregion

        public ComboBox() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnDropDownClosed(Windows.Foundation.Object e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnDropDownOpened(Windows.Foundation.Object e)
        {
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CComboBoxTextSubmittedEventArgs")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "9830c68a-fc2d-424b-84c0-3f5028908e83")]
    public sealed class ComboBoxTextSubmittedEventArgs
    : Microsoft.UI.Xaml.EventArgs
    {
        internal ComboBoxTextSubmittedEventArgs() { }

        [ReadOnly]
        [OffsetFieldName("m_itemToCommit")]
        [DelegateToCore]
        public Windows.Foundation.String Text
        {
            get;
            internal set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public bool Handled
        {
            get;
            set;
        }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true)]
    public enum ComboBoxSelectionChangedTrigger
    {
        Committed = 0,
        Always = 1,
    }
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IScrollItemProvider))]
    [Guids(ClassGuid = "b4819bd4-2d9e-4f4d-b8c7-dd6406627e3b")]
    public class ComboBoxItemDataAutomationPeer
 : Microsoft.UI.Xaml.Automation.Peers.SelectorItemAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithParentAndItem")]
        public ComboBoxItemDataAutomationPeer(Windows.Foundation.Object item, Microsoft.UI.Xaml.Automation.Peers.ComboBoxAutomationPeer parent)
            : base(item, parent) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "9b06a419-33b1-4d63-b5b2-78f514f66c01")]
    public class ComboBoxItemAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ComboBoxItemAutomationPeer(Microsoft.UI.Xaml.Controls.ComboBoxItem owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IValueProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IWindowProvider), Version = 1)]
    [Guids(ClassGuid = "a01d529c-37f4-4d9d-b37a-928e00e8ec8c")]
    public class ComboBoxAutomationPeer
 : Microsoft.UI.Xaml.Automation.Peers.SelectorAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public ComboBoxAutomationPeer(Microsoft.UI.Xaml.Controls.ComboBox owner)
            : base(owner) { }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Guids(ClassGuid = "d1260b64-6d40-436e-9cb9-c7d6d6e6fb7e")]
    internal sealed class ComboBoxLightDismissAutomationPeer
 : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {

    }
}
