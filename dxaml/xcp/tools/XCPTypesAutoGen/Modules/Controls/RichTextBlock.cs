// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "8f1f4005-7e25-4daa-b0d2-e0508079f130")]
    [NativeName("CRichTextBlock")]
    [ContentProperty("Blocks")]
    [ClassFlags(CanConvertFromString = true)]
    public sealed class RichTextBlock
        : Microsoft.UI.Xaml.FrameworkElement
    {
        #region Properties

        // Version 1
        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_eFontSize")]
        public Windows.Foundation.Double FontSize
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily FontFamily
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontWeight")]
        [CoreType(typeof(Windows.UI.Text.CoreFontWeight), NewCodeGenPropertyType = typeof(Windows.UI.Text.FontWeight))]
        public Windows.UI.Text.FontWeight FontWeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStyle")]
        public Windows.UI.Text.FontStyle FontStyle
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStretch")]
        public Windows.UI.Text.FontStretch FontStretch
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pForeground")]
        public Microsoft.UI.Xaml.Media.Brush Foreground
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

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textTrimming")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextTrimming TextTrimming
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
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
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBlocks")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.BlockCollection Blocks
        {
            get;
            private set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_padding")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eLineHeight")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double LineHeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_lineStackingStrategy")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.LineStackingStrategy LineStackingStrategy
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nCharacterSpacing")]
        public Windows.Foundation.Int32 CharacterSpacing
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsArrange = true, DoNotEnterOrLeaveValue = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pOverflowTarget")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Controls.RichTextBlockOverflow OverflowContentTarget
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isTextSelectionEnabled")]
        public Windows.Foundation.Boolean IsTextSelectionEnabled
        {
            get;
            set;
        }

        [NativeMethod("CRichTextBlock", "HasOverflowContent")]
        [NativeStorageType(ValueType.valueBool)]
        [ReadOnly]
        public Windows.Foundation.Boolean HasOverflowContent
        {
            get;
            private set;
        }

        [NativeMethod("CRichTextBlock", "GetSelectedText")]
        [NativeStorageType(ValueType.valueString)]
        [ReadOnly]
        public Windows.Foundation.String SelectedText
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.TextPointer ContentStart
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.TextPointer ContentEnd
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.TextPointer SelectionStart
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.TextPointer SelectionEnd
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double BaselineOffset
        {
            get;
            private set;
        }

        [OrderHint(1)]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_textIndent")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double TextIndent
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_maxLines")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Int32 MaxLines
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textLineBounds")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextLineBounds TextLineBounds
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

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_opticalMarginAlignment")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.OpticalMarginAlignment OpticalMarginAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isColorFontEnabled")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Boolean IsColorFontEnabled
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

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_isTextScaleFactorEnabled")]
        public Windows.Foundation.Boolean IsTextScaleFactorEnabled
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nTextDecorations")]
        public Windows.UI.Text.TextDecorations TextDecorations
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isTextTrimmed")]
        public Windows.Foundation.Boolean IsTextTrimmed
        {
            get;
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

        [CollectionType(CollectionKind.Vector)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsValueCreatedOnDemand = true, IsReadOnlyExceptForParser = true, IsExcludedFromVisualTree = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_textHighlighters")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Documents.TextHighlighterCollection TextHighlighters
        {
            get;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase SelectionFlyout
        {
            get;
            set;
        }

        #endregion

        #region Events

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler SelectionChanged;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.ContextMenuOpeningEventHandler ContextMenuOpening;

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichTextBlock, IsTextTrimmedChangedEventArgs> IsTextTrimmedChanged;

        #endregion

        #region Methods

        [PInvoke]
        public void SelectAll()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void Select(Microsoft.UI.Xaml.Documents.TextPointer start, Microsoft.UI.Xaml.Documents.TextPointer end)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Documents.TextPointer GetPositionFromPoint(Windows.Foundation.Point point)
        {
            return default(Microsoft.UI.Xaml.Documents.TextPointer);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void CopySelectionToClipboard()
        {
        }

        #endregion

        public RichTextBlock() { }
    }
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [NativeName("CRichTextBlockAutomationPeer")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "53107323-b33c-41dd-8d2e-58ea8594573b")]
    public class RichTextBlockAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public RichTextBlockAutomationPeer(Microsoft.UI.Xaml.Controls.RichTextBlock owner)
            : base(owner) { }
    }
}
