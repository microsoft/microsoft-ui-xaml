// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Windows.Foundation;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Documents
{
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "012e43a8-2a9f-4bb9-8da7-554da9a2f147")]
    [NativeName("CTextElement")]
    public abstract class TextElement
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
        public new Windows.Foundation.String Name
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_eFontSize")]
        public Windows.Foundation.Double FontSize
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily FontFamily
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontWeight")]
        [CoreType(typeof(Windows.UI.Text.CoreFontWeight), NewCodeGenPropertyType = typeof(Windows.UI.Text.FontWeight))]
        public Windows.UI.Text.FontWeight FontWeight
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStyle")]
        public Windows.UI.Text.FontStyle FontStyle
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStretch")]
        public Windows.UI.Text.FontStretch FontStretch
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
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
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pForeground")]
        public Microsoft.UI.Xaml.Media.Brush Foreground
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_strLanguageString")]
        public Windows.Foundation.String Language
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

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nTextDecorations")]
        public Windows.UI.Text.TextDecorations TextDecorations
        {
            get;
            set;
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
        public Microsoft.UI.Xaml.Documents.TextPointer ElementStart
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.TextPointer ElementEnd
        {
            get;
            private set;
        }

        internal TextElement() { }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [MethodFlags(IsImplVirtual = true)]
        protected virtual Microsoft.UI.Xaml.Automation.Peers.AutomationPeer OnCreateAutomationPeer()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }

        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDisconnectVisualChildren()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [NativeClassName("CTextElement")]
        public Windows.Foundation.Object FindName(Windows.Foundation.String name)
        {
            return default(Windows.Foundation.Object);
        }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Boolean AllowFocusOnInteraction
        {
            get;
            set;
        }

        public Windows.Foundation.String AccessKey
        {
            get;
            set;
        }

        public bool ExitDisplayModeOnAccessKeyInvoked
        {
            get;
            set;
        }

        public bool IsAccessKeyScope
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject AccessKeyScopeOwner
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.KeyTipPlacementMode KeyTipPlacementMode
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Double KeyTipHorizontalOffset
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Double KeyTipVerticalOffset
        {
            get;
            set;
        }

        [EventFlags(UseEventManager = true)]
        public event TypedEventHandler<TextElement, AccessKeyDisplayRequestedEventArgs> AccessKeyDisplayRequested;

        [EventFlags(UseEventManager = true)]
        public event TypedEventHandler<TextElement, AccessKeyDisplayDismissedEventArgs> AccessKeyDisplayDismissed;

        [EventFlags(UseEventManager = true)]
        public event TypedEventHandler<TextElement, AccessKeyInvokedEventArgs> AccessKeyInvoked;

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.XamlRoot XamlRoot {set; get; }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CTextElementCollection")]
    [ClassFlags(IsHiddenFromIdl = true)]
    [HideFromOldCodeGen]
    [HandWritten]
    [Guids(ClassGuid = "8d52996d-f33d-4fd7-8e86-4fb707df4812")]
    public abstract class TextElementCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<TextElement>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Documents.TextElement ContentProperty
        {
            get;
            set;
        }

        protected TextElementCollection() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CTextElementCollection")]
    [ClassFlags(IsHiddenFromIdl = true)]
    [OldCodeGenBaseType(typeof(Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<TextElement>))]
    public abstract class TextElementCollection<T>
     : TextElementCollection
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [HideFromNewCodeGen]
        public new Microsoft.UI.Xaml.Documents.TextElement ContentProperty
        {
            get;
            set;
        }

        protected TextElementCollection() { }
    }

    [NativeName("CInline")]
    [Guids(ClassGuid = "cd0b81bb-e5f7-46b8-8bb8-000b01843ac1")]
    public abstract class Inline
     : Microsoft.UI.Xaml.Documents.TextElement
    {
        protected Inline() { }
    }

    [NativeName("CBlock")]
    [Guids(ClassGuid = "23b8c620-2419-40a2-859b-5e3422b45c5d")]
    public abstract class Block
     : Microsoft.UI.Xaml.Documents.TextElement
    {

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textAlignment")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.TextAlignment TextAlignment
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_textAlignment")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.TextAlignment HorizontalTextAlignment
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_lineHeight")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double LineHeight
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_lineStackingStrategy")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.LineStackingStrategy LineStackingStrategy
        {
            get;
            set;
        }

        [OrderHint(1)]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_margin")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Thickness Margin
        {
            get;
            set;
        }

        protected Block() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CInlineCollection")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.Documents.TextElementCollection<Inline>))]
    [ClassFlags(IsWhitespaceSignificant = true, HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "c13ae71b-0fe7-4d63-95a8-6a2d35aba929")]
    public sealed class InlineCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Inline>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Documents.Inline ContentProperty
        {
            get;
            set;
        }

        internal InlineCollection() { }
    }

    [CodeGen(partial: true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CBlockCollection")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.Documents.TextElementCollection<Block>))]
    [Guids(ClassGuid = "609e3823-be19-4d04-aab8-2c028308f9be")]
    public sealed class BlockCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Block>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Documents.Block ContentProperty
        {
            get;
            set;
        }

        internal BlockCollection() { }
    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("CGlyphs")]
    [Guids(ClassGuid = "a1ba5ea2-aee8-4085-b254-6c97a8de1afe")]
    public sealed class Glyphs
     : Microsoft.UI.Xaml.FrameworkElement
    {

        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strUnicodeString")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.String UnicodeString
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strIndices")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.String Indices
        {
            get;
            set;
        }

        [PropertyFlags(NeedsInvoke = true)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strFontUri")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Uri FontUri
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nStyleSimulations")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.StyleSimulations StyleSimulations
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eFontRenderingEmSize")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double FontRenderingEmSize
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eOriginX")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double OriginX
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eOriginY")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double OriginY
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pFill")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush Fill
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

        [OffsetFieldName("m_colorPaletteIndex")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Windows.Foundation.Int32 ColorFontPaletteIndex
        {
            get;
            set;
        }

        public Glyphs() { }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeClassName("CGlyphs")]
        public void SetFontSource(Microsoft.UI.Xaml.DependencyObject downloader, Windows.Foundation.String partName)
        {
        }
    }

    [NativeName("CRun")]
    [ContentProperty("Text")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "931868a3-bfd3-4c3d-a2b7-3a23e64bf790")]
    public sealed class Run
     : Microsoft.UI.Xaml.Documents.Inline
    {

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strText")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.String Text
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFlowDirection")]
        public Microsoft.UI.Xaml.FlowDirection FlowDirection
        {
            get;
            set;
        }

        public Run() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CParagraph")]
    [ContentProperty("Inlines")]
    [Guids(ClassGuid = "e5be8404-0542-40ac-bc21-3feb69d11f3d")]
    public sealed class Paragraph
     : Microsoft.UI.Xaml.Documents.Block
    {
        [PropertyFlags(AffectsMeasure = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pInlines")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.InlineCollection Inlines
        {
            get;
            private set;
        }

        [OrderHint(1)]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_textIndent")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Windows.Foundation.Double TextIndent
        {
            get;
            set;
        }

        public Paragraph() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CSpan")]
    [ContentProperty("Inlines")]
    [Guids(ClassGuid = "e1e27d64-0bb7-4f89-abdc-f8d480491b6b")]
    public class Span
     : Microsoft.UI.Xaml.Documents.Inline
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pInlines")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Documents.InlineCollection Inlines
        {
            get;
            set;
        }

        public Span() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [NativeName("CUnderline")]
    [Guids(ClassGuid = "4aa23829-f883-4798-a663-84e9094ce85a")]
    public sealed class Underline
     : Microsoft.UI.Xaml.Documents.Span
    {
        public Underline() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [NativeName("CItalic")]
    [Guids(ClassGuid = "625d6594-1d04-472b-8de7-7266fd7a6918")]
    public sealed class Italic
     : Microsoft.UI.Xaml.Documents.Span
    {
        public Italic() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [NativeName("CBold")]
    [Guids(ClassGuid = "e1ed42af-044a-4295-84a4-f644a65e1df4")]
    public sealed class Bold
     : Microsoft.UI.Xaml.Documents.Span
    {
        public Bold() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CHyperlink")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "a30a93ea-2114-4001-8093-b1b354950758")]
    public sealed class Hyperlink
     : Microsoft.UI.Xaml.Documents.Span
    {
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strNavigateUri")]
        public Windows.Foundation.Uri NavigateUri
        {
            get;
            set;
        }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Documents.HyperlinkClickEventHandler Click;

        public Hyperlink() { }


        public Windows.Foundation.Boolean Focus(Microsoft.UI.Xaml.FocusState value)
        {
            return default(Windows.Foundation.Boolean);
        }

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler GotFocus;

        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler LostFocus;

        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Documents.UnderlineStyle UnderlineStyle
        {
            get;
            set;
        }

        protected override void OnDisconnectVisualChildren()
        {
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusLeft
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusRight
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusUp
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusDown
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.ElementSoundMode ElementSoundMode
        {
            get;
            set;
        }

        [ReadOnly]
        public Microsoft.UI.Xaml.FocusState FocusState
        {
            get;
            private set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusUpNavigationStrategy
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusDownNavigationStrategy
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusLeftNavigationStrategy
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusRightNavigationStrategy
        {
            get;
            set;
        }

        [OffsetFieldName("m_isTabStop")]
        public Windows.Foundation.Boolean IsTabStop
        {
            get;
            set;
        }

        public Windows.Foundation.Int32 TabIndex
        {
            get;
            set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CHyperlinkClickEventArgs")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "844ad7c1-76ea-4e7c-bfab-8132700f9f5a")]
    public sealed class HyperlinkClickEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        internal HyperlinkClickEventArgs() { }
    }
    
    [NativeName("CLineBreak")]
    [ClassFlags(TrimSurroundingWhitespace = true)]
    [Guids(ClassGuid = "b807a1f7-a0d5-4680-aed1-470cd347e386")]
    public sealed class LineBreak
     : Microsoft.UI.Xaml.Documents.Inline
    {
        public LineBreak() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [InterfaceDetails(ClassGuid = "a482300a-3e35-4620-aeed-bae246bd3845")]
    public class TextHighlighterBase : DependencyObject
    {
        internal TextHighlighterBase() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CTextHighlighter")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [InterfaceDetails(ClassGuid = "adce57f4-4ae7-4f92-a19b-04c186bdcbe3")]
    public class TextHighlighter : TextHighlighterBase
    {
        public TextHighlighter() { }

        [CollectionType(CollectionKind.Vector)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsValueCreatedOnDemand = true, IsReadOnlyExceptForParser = true, IsExcludedFromVisualTree = true)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_ranges")]
        public TextRangeCollection Ranges { get; }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_foreground")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Brush Foreground { get; set; }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_background")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Media.Brush Background { get; set; }
    }

    [BuiltinStruct("CTextRange")]
    public struct TextRange
    {
        [CoreType(typeof(Windows.Foundation.Int32))]
        [NativeStorageType(OM.ValueType.valueSigned)]
        [OffsetFieldName("m_range.startIndex")]
        public Windows.Foundation.Int32 StartIndex { get; set; }

        [CoreType(typeof(Windows.Foundation.Int32))]
        [NativeStorageType(OM.ValueType.valueSigned)]
        [OffsetFieldName("m_range.length")]
        public Windows.Foundation.Int32 Length { get; set; }
    }

    [CodeGen(partial: true)]
    [ClassFlags(IsHiddenFromIdl = true, HasBaseTypeInDXamlInterface = false)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextRangeCollection")]
    [InterfaceDetails(ClassGuid = "e930ef7a-2524-4844-b74f-d829ec42eab0")]
    public sealed class TextRangeCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<TextRange>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Documents.TextRange ContentProperty
        {
            get;
            set;
        }

        internal TextRangeCollection() { }
    }

    [CodeGen(partial: true)]
    [ClassFlags(IsHiddenFromIdl = true, HasBaseTypeInDXamlInterface = false)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextHighlighterCollection")]
    [InterfaceDetails(ClassGuid = "924121f4-cb70-4966-9748-6b6a90d47a99")]
    public sealed class TextHighlighterCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<TextHighlighter>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.Documents.TextHighlighter ContentProperty
        {
            get;
            set;
        }

        internal TextHighlighterCollection() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CInlineUIContainer")]
    [ContentProperty("Child")]
    [Guids(ClassGuid = "315ffaef-b154-4442-acfb-699f6e130b27")]
    public sealed class InlineUIContainer
     : Microsoft.UI.Xaml.Documents.Inline
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pChild")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.UIElement Child
        {
            get;
            set;
        }

        public InlineUIContainer() { }

        protected override void OnDisconnectVisualChildren()
        {
        }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "d15b8a26-fad2-4411-a943-052637bc6d68")]
    public sealed class TextPointer
     : Windows.Foundation.Object
    {
        [CodeGen(CodeGenLevel.Idl)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Microsoft.UI.Xaml.Internal.TextPointerWrapper InternalPointer
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.DependencyObject Parent
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.FrameworkElement VisualParent
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.LogicalDirection LogicalDirection
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Int32 Offset
        {
            get;
            private set;
        }

        internal TextPointer() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Rect GetCharacterRect(Microsoft.UI.Xaml.Documents.LogicalDirection direction)
        {
            return default(Windows.Foundation.Rect);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Documents.TextPointer GetPositionAtOffset(Windows.Foundation.Int32 offset, Microsoft.UI.Xaml.Documents.LogicalDirection direction)
        {
            return default(Microsoft.UI.Xaml.Documents.TextPointer);
        }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1, ForcePrimaryInterfaceGeneration = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Comment("Unmanaged stub for managed-only typography properties.")]
    [NativeCreationMethodName("")]
    [Guids(ClassGuid = "ff05bf9f-192c-48f1-bd3a-0cc927355bac")]
    public static class Typography
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_nAnnotationAlternates")]
        public static Windows.Foundation.Int32 AttachedAnnotationAlternates
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fEastAsianExpertForms")]
        public static Windows.Foundation.Boolean AttachedEastAsianExpertForms
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_EastAsianLanguage")]
        public static Microsoft.UI.Xaml.FontEastAsianLanguage AttachedEastAsianLanguage
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_EastAsianWidths")]
        public static Microsoft.UI.Xaml.FontEastAsianWidths AttachedEastAsianWidths
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStandardLigatures")]
        public static Windows.Foundation.Boolean AttachedStandardLigatures
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fContextualLigatures")]
        public static Windows.Foundation.Boolean AttachedContextualLigatures
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fDiscretionaryLigatures")]
        public static Windows.Foundation.Boolean AttachedDiscretionaryLigatures
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fHistoricalLigatures")]
        public static Windows.Foundation.Boolean AttachedHistoricalLigatures
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_nStandardSwashes")]
        public static Windows.Foundation.Int32 AttachedStandardSwashes
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_nContextualSwashes")]
        public static Windows.Foundation.Int32 AttachedContextualSwashes
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fContextualAlternates")]
        public static Windows.Foundation.Boolean AttachedContextualAlternates
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_nStylisticAlternates")]
        public static Windows.Foundation.Int32 AttachedStylisticAlternates
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet1")]
        public static Windows.Foundation.Boolean AttachedStylisticSet1
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet2")]
        public static Windows.Foundation.Boolean AttachedStylisticSet2
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet3")]
        public static Windows.Foundation.Boolean AttachedStylisticSet3
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet4")]
        public static Windows.Foundation.Boolean AttachedStylisticSet4
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet5")]
        public static Windows.Foundation.Boolean AttachedStylisticSet5
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet6")]
        public static Windows.Foundation.Boolean AttachedStylisticSet6
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet7")]
        public static Windows.Foundation.Boolean AttachedStylisticSet7
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet8")]
        public static Windows.Foundation.Boolean AttachedStylisticSet8
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet9")]
        public static Windows.Foundation.Boolean AttachedStylisticSet9
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet10")]
        public static Windows.Foundation.Boolean AttachedStylisticSet10
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet11")]
        public static Windows.Foundation.Boolean AttachedStylisticSet11
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet12")]
        public static Windows.Foundation.Boolean AttachedStylisticSet12
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet13")]
        public static Windows.Foundation.Boolean AttachedStylisticSet13
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet14")]
        public static Windows.Foundation.Boolean AttachedStylisticSet14
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet15")]
        public static Windows.Foundation.Boolean AttachedStylisticSet15
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet16")]
        public static Windows.Foundation.Boolean AttachedStylisticSet16
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet17")]
        public static Windows.Foundation.Boolean AttachedStylisticSet17
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet18")]
        public static Windows.Foundation.Boolean AttachedStylisticSet18
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet19")]
        public static Windows.Foundation.Boolean AttachedStylisticSet19
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fStylisticSet20")]
        public static Windows.Foundation.Boolean AttachedStylisticSet20
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_Capitals")]
        public static Microsoft.UI.Xaml.FontCapitals AttachedCapitals
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fCapitalSpacing")]
        public static Windows.Foundation.Boolean AttachedCapitalSpacing
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fKerning")]
        public static Windows.Foundation.Boolean AttachedKerning
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fCaseSensitiveForms")]
        public static Windows.Foundation.Boolean AttachedCaseSensitiveForms
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fHistoricalForms")]
        public static Windows.Foundation.Boolean AttachedHistoricalForms
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_Fraction")]
        public static Microsoft.UI.Xaml.FontFraction AttachedFraction
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_NumeralStyle")]
        public static Microsoft.UI.Xaml.FontNumeralStyle AttachedNumeralStyle
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_NumeralAlignment")]
        public static Microsoft.UI.Xaml.FontNumeralAlignment AttachedNumeralAlignment
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fSlashedZero")]
        public static Windows.Foundation.Boolean AttachedSlashedZero
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_fMathematicalGreek")]
        public static Windows.Foundation.Boolean AttachedMathematicalGreek
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pInheritedProperties")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        [StorageGroupNames("EnsureInheritedProperties", "InheritedProperties", "m_typography.m_Variants")]
        public static Microsoft.UI.Xaml.FontVariants AttachedVariants
        {
            get;
            set;
        }
    }

    [TypeTable(IsExcludedFromCore = true)]
    public enum LogicalDirection
    {
        Backward = 0,
        Forward = 1,
    }

    public enum UnderlineStyle
    {
        None = 0,
        Single = 1,
    }

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "0f010912-5af3-48fa-b223-5b3c471f62c4")]
    public delegate void HyperlinkClickEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Documents.HyperlinkClickEventArgs e);

}
