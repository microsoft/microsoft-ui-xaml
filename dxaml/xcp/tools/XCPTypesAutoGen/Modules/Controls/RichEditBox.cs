// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    #region Enums
    
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    public enum RichEditClipboardFormat
    {
        [NativeValueName("RichEditClipboardFormatAllFormats")]
        AllFormats = 0,
        [NativeValueName("RichEditClipboardFormatPlainText")]
        PlainText = 1,
    }

    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, AreValuesFlags = true)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    public enum DisabledFormattingAccelerators
    {
        None = 0,
        Bold = 1,
        Italic = 2,
        Underline = 4,
        All = -1
    }
    
    #endregion

    [CodeGen(partial: true)]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "a2752cbb-f8d0-46f1-96d1-9c900da05b12")]
    [NativeName("CRichEditBox")]
    [DXamlIdlGroup("Controls2")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ITelemetryCollectionPriv))]
    public class RichEditBox : Microsoft.UI.Xaml.Controls.Control
    {
        #region Properties

        // Version 1
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bIsReadOnly")]
        public Windows.Foundation.Boolean IsReadOnly
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bAcceptsReturn")]
        public Windows.Foundation.Boolean AcceptsReturn
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textAlignment")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextAlignment TextAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textWrapping")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextWrapping TextWrapping
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isSpellCheckEnabled")]
        public Windows.Foundation.Boolean IsSpellCheckEnabled
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isTextPredictionEnabled")]
        public Windows.Foundation.Boolean IsTextPredictionEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Text.RichEditTextDocument Document
        {
            get;
            private set;
        }

        [RequiresMultipleAssociationCheck]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pInputScope")]
        public Microsoft.UI.Xaml.Input.InputScope InputScope
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

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isColorFontEnabled")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Boolean IsColorFontEnabled
        {
            get;
            set;
        }

        [OrderHint(2)]
        public CandidateWindowAlignment DesiredCandidateWindowAlignment
        {
            get;
            set;
        }

        [OrderHint(1)]
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

        [OrderHint(1)]
        public RichEditClipboardFormat ClipboardCopyFormat
        {
            get;
            set;
        }

        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectionHighlightColorWhenNotFocused")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.SolidColorBrush SelectionHighlightColorWhenNotFocused
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

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textAlignment")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextAlignment HorizontalTextAlignment
        {
            get;
            set;
        }

        public CharacterCasing CharacterCasing
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        public DisabledFormattingAccelerators DisabledFormattingAccelerators
        {
            get;
            set;
        }
        
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Text.RichEditTextDocument TextDocument
        {
            get;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase SelectionFlyout
        {
            get;
            set;
        }

        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [NativeMethod("CTextBoxBase", "GetProofingMenuFlyout")]
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase ProofingMenuFlyout
        {
            get;
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

        // Version 1
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler TextChanged;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler SelectionChanged;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.ContextMenuOpeningEventHandler ContextMenuOpening;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.TextControlPasteEventHandler Paste;

        [OrderHint(2)]
        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, CandidateWindowBoundsChangedEventArgs> CandidateWindowBoundsChanged;

        [OrderHint(2)]
        [EventFlags(UseEventManager = false)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, RichEditBoxTextChangingEventArgs> TextChanging;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, TextCompositionStartedEventArgs> TextCompositionStarted;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, TextCompositionChangedEventArgs> TextCompositionChanged;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, TextCompositionEndedEventArgs> TextCompositionEnded;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, TextControlCopyingToClipboardEventArgs> CopyingToClipboard;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, TextControlCuttingToClipboardEventArgs> CuttingToClipboard;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichEditBox, RichEditBoxSelectionChangingEventArgs> SelectionChanging;

        #endregion

        #region Methods

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Windows.Foundation.Collections.IVectorView<Windows.Foundation.String>> GetLinguisticAlternativesAsync()
        {
            return default(Windows.Foundation.IAsyncOperation<Windows.Foundation.Collections.IVectorView<Windows.Foundation.String>>);
        }

        #endregion

        public RichEditBox() { }
    }

    #region EventArgs

    [Guids(ClassGuid = "9e47438c-e179-4185-9231-a0ee71def417")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CRichEditBoxTextChangingEventArgs")]
    [DXamlIdlGroup("Controls2")]
    public sealed class RichEditBoxTextChangingEventArgs
        : Microsoft.UI.Xaml.EventArgs
    {
        internal RichEditBoxTextChangingEventArgs() { }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean IsContentChanging
        {
            get;
            internal set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "e74cbfe9-0a3f-4aed-91ea-2424769cc969")]
    [DXamlIdlGroup("Controls2")]
    public sealed class RichEditBoxSelectionChangingEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        public Windows.Foundation.Int32 SelectionStart
        {
            get;
            internal set;
        }

        public Windows.Foundation.Int32 SelectionLength
        {
            get;
            internal set;
        }

        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        internal RichEditBoxSelectionChangingEventArgs() { }
    }

    #endregion
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CRichEditBoxAutomationPeer")]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "90ace321-b1d9-4710-b50f-fec4f32f2e87")]
    public class RichEditBoxAutomationPeer : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public RichEditBoxAutomationPeer(Microsoft.UI.Xaml.Controls.RichEditBox owner)
            : base(owner) { }
    }
}
