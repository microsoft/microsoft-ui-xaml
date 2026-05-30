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
    [Guids(ClassGuid = "67027aca-57cd-4690-85b9-823162cd6eff")]
    [NativeName("CTextBox")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ITextBoxPriv2))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ITelemetryCollectionPriv))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl), Velocity = "Feature_InputValidation")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInputValidationControl2), Velocity = "Feature_InputValidation")]
    [InputProperty("Text")]
    public class TextBox : Microsoft.UI.Xaml.Controls.Control
    {
        #region Properties

        // Version 1
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strText")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.String Text
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strSelectedText")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Windows.Foundation.String SelectedText
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_iSelectionLength")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Windows.Foundation.Int32 SelectionLength
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_iSelectionStart")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Windows.Foundation.Int32 SelectionStart
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

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isDesktopPopupMenuEnabled")]
        internal Windows.Foundation.Boolean IsCoreDesktopPopupMenuEnabled
        {
            get;
            set;
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

        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PlaceholderForeground
        {
            get;
            set;
        }

        [NativeMethod("CTextBox", "CanPasteClipboardContent")]
        [NativeStorageType(ValueType.valueBool)]
        public Windows.Foundation.Boolean CanPasteClipboardContent
        {
            get;
        }

        [NativeMethod("CTextBox", "CanUndo")]
        [NativeStorageType(ValueType.valueBool)]
        public Windows.Foundation.Boolean CanUndo
        {
            get;
        }

        [NativeMethod("CTextBox", "CanRedo")]
        [NativeStorageType(ValueType.valueBool)]
        public Windows.Foundation.Boolean CanRedo
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
        public event Microsoft.UI.Xaml.Controls.TextChangedEventHandler TextChanged;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler SelectionChanged;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.ContextMenuOpeningEventHandler ContextMenuOpening;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.TextControlPasteEventHandler Paste;

        [OrderHint(2)]
        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, CandidateWindowBoundsChangedEventArgs> CandidateWindowBoundsChanged;

        [OrderHint(2)]
        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextBoxTextChangingEventArgs> TextChanging;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextCompositionStartedEventArgs> TextCompositionStarted;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextCompositionChangedEventArgs> TextCompositionChanged;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextCompositionEndedEventArgs> TextCompositionEnded;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextControlCopyingToClipboardEventArgs> CopyingToClipboard;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextControlCuttingToClipboardEventArgs> CuttingToClipboard;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextBoxBeforeTextChangingEventArgs> BeforeTextChanging;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<TextBox, TextBoxSelectionChangingEventArgs> SelectionChanging;

        #endregion

        #region Methods

        // Version 1
        [PInvoke]
        public void Select(Windows.Foundation.Int32 start, Windows.Foundation.Int32 length)
        {
        }

        [PInvoke]
        public void SelectAll()
        {
        }

        [PInvoke]
        public Windows.Foundation.Rect GetRectFromCharacterIndex(Windows.Foundation.Int32 charIndex, Windows.Foundation.Boolean trailingEdge)
        {
            return default(Windows.Foundation.Rect);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Windows.Foundation.Collections.IVectorView<Windows.Foundation.String>> GetLinguisticAlternativesAsync()
        {
            return default(Windows.Foundation.IAsyncOperation<Windows.Foundation.Collections.IVectorView<Windows.Foundation.String>>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Undo()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Redo()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PasteFromClipboard()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void CopySelectionToClipboard()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void CutSelectionToClipboard()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ClearUndoRedoHistory()
        {
        }

        #endregion

        public TextBox() { }
    }

    #region EventArgs

    [Guids(ClassGuid = "239f56d3-473f-4ff8-a69b-ae869e20dbe3")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextBoxTextChangingEventArgs")]
    public sealed class TextBoxTextChangingEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        internal TextBoxTextChangingEventArgs() { }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean IsContentChanging
        {
            get;
            internal set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "1f3e607a-47a2-4936-b7bf-43d654122d62")]
    public sealed class TextBoxBeforeTextChangingEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromCore = true)]
        public string NewText
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        internal TextBoxBeforeTextChangingEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "787c0b69-4483-42cf-b4b1-4517034818d7")]
    public sealed class TextBoxSelectionChangingEventArgs : Microsoft.UI.Xaml.EventArgs
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

        internal TextBoxSelectionChangingEventArgs() { }

    }

    #endregion
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextBoxBaseAutomationPeer")]
    [Guids(ClassGuid = "81dea851-4dba-433a-aee1-670cdb0aceb0")]
    internal abstract class TextBoxBaseAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        protected TextBoxBaseAutomationPeer() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [NativeName("CTextBoxAutomationPeer")]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "52f31952-ba95-446b-8b0c-3082785aae4c")]
    public class TextBoxAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public TextBoxAutomationPeer(Microsoft.UI.Xaml.Controls.TextBox owner)
            : base(owner) { }
    }
}
