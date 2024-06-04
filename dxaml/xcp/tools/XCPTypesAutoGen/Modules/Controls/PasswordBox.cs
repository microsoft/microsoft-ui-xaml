// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    #region Enums

    [NativeName("PasswordRevealMode")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    [NativeComment("Determines the reveal mode for password input.")]
    public enum PasswordRevealMode
    {
        [NativeValueName("Peek")]
        Peek = 0,
        [NativeValueName("Hidden")]
        Hidden = 1,
        [NativeValueName("Visible")]
        Visible = 2,
    }

    #endregion

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Platform("Feature_InputValidation", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "00be1d81-4ab1-4c64-8507-dd8387fc3cfa")]
    [NativeName("CPasswordBox")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl), Velocity = "Feature_InputValidation")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl2), Velocity = "Feature_InputValidation")]
    [InputProperty("Password")]
    public sealed class PasswordBox
        : Microsoft.UI.Xaml.Controls.Control
    {
        #region Properties

        // Version 1
        [NativeMethod("CPasswordBox", "Password")]
        public Windows.Foundation.String Password
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strPasswordChar")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.String PasswordChar
        {
            get;
            set;
        }

        [Deprecated("IsPasswordRevealButtonEnabledProperty may be altered or unavailable for releases after Windows 10.0. Instead, use PasswordRevealModeProperty.")]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fRevealButtonEnabled")]
        public Windows.Foundation.Boolean IsPasswordRevealButtonEnabled
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_iMaxLength")]
        public Windows.Foundation.Int32 MaxLength
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        public Windows.Foundation.Object Header
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        public Windows.Foundation.String PlaceholderText
        {
            get;
            set;
        }

        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectionHighlightColor")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.SolidColorBrush SelectionHighlightColor
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bPreventKeyboardDisplayOnProgrammaticFocus")]
        public Windows.Foundation.Boolean PreventKeyboardDisplayOnProgrammaticFocus
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_passwordRevealMode")]
        public PasswordRevealMode PasswordRevealMode
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textReadingOrder")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextReadingOrder TextReadingOrder
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [RequiresMultipleAssociationCheck]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pInputScope")]
        public Microsoft.UI.Xaml.Input.InputScope InputScope
        {
            get;
            set;
        }

        [NativeMethod("CPasswordBox", "CanPasteClipboardContent")]
        [NativeStorageType(ValueType.valueBool)]
        public Windows.Foundation.Boolean CanPasteClipboardContent
        {
            get;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase SelectionFlyout
        {
            get;
            set;
        }

        public object Description
        {
            get;
            set;
        }

        // Velocity
        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement
        {
            get;
            set;
        }

        #endregion

        #region Events

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler PasswordChanged;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.ContextMenuOpeningEventHandler ContextMenuOpening;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.TextControlPasteEventHandler Paste;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<PasswordBox, PasswordBoxPasswordChangingEventArgs> PasswordChanging;

        #endregion

        #region Methods

        [PInvoke]
        public void SelectAll()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PasteFromClipboard()
        {
        }

        #endregion

        public PasswordBox() { }
    }

    #region EventArgs

    [Guids(ClassGuid = "06e29d53-4b01-496e-a69c-633be23c032a")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [TypeFlags(IsCreateableFromXAML = false)]
    public sealed class PasswordBoxPasswordChangingEventArgs
        : Microsoft.UI.Xaml.EventArgs
    {
        internal PasswordBoxPasswordChangingEventArgs() { }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean IsContentChanging
        {
            get;
            internal set;
        }
    }

    #endregion
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [NativeName("CPasswordBoxAutomationPeer")]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "b01161e3-5171-48e9-95b0-c37bce3f53d9")]
    public class PasswordBoxAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public PasswordBoxAutomationPeer(Microsoft.UI.Xaml.Controls.PasswordBox owner)
            : base(owner) { }
    }
}
