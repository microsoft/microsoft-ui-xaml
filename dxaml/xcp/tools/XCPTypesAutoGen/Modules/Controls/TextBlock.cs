// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "084b23b7-41a9-44c6-a55d-f16b809e0dcf")]
    [NativeName("CTextBlock")]
    [ContentProperty("Inlines")]
    public sealed class TextBlock
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

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nCharacterSpacing")]
        public Windows.Foundation.Int32 CharacterSpacing
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

        [PropertyFlags(AffectsMeasure = true)]
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

        [PropertyFlags(AffectsMeasure = true, IsValueCreatedOnDemand = true)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strText")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.String Text
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pInlines")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Documents.InlineCollection Inlines
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

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isTextSelectionEnabled")]
        public Windows.Foundation.Boolean IsTextSelectionEnabled
        {
            get;
            set;
        }

        [NativeMethod("CTextBlock", "GetSelectedText")]
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

        [PropertyFlags(IsValueCreatedOnDemand = true, HadFieldInBlue = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.SolidColorBrush SelectionHighlightColor
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

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [CollectionType(CollectionKind.Vector)]
        [DependencyPropertyModifier(Modifier.Private)]
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
        public event Windows.Foundation.TypedEventHandler<TextBlock, IsTextTrimmedChangedEventArgs> IsTextTrimmedChanged;

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
        public Microsoft.UI.Composition.CompositionBrush GetAlphaMask()
        {
            return default(Microsoft.UI.Composition.CompositionBrush);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void CopySelectionToClipboard()
        {
        }

        #endregion

        public TextBlock() { }
    }
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [NativeName("CTextBlockAutomationPeer")]
    [Guids(ClassGuid = "091308ab-0a81-4956-9288-de1bf67b5ba9")]
    public class TextBlockAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public TextBlockAutomationPeer(Microsoft.UI.Xaml.Controls.TextBlock owner)
            : base(owner) { }

        public override Windows.Foundation.Object GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface patternInterface)
        {
            return default(Windows.Foundation.Object);
        }
    }
}
