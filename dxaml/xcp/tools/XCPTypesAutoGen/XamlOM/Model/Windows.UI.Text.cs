// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.UI.Text
{
    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [Platform(typeof(Windows.Foundation.UniversalApiContract), 1)]
    [TypeFlags(IsRemappingOfCoreType = true, IsWebHostHidden = false)]
    [ClassFlags(HasTypeConverter = true)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [HandWritten(InCore = true)]
    public struct FontWeight
    {
        public Windows.Foundation.UInt16 Weight
        {
            get;
            set;
        }
    }

    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("FontWeight")]
    [TypeTableName("FontWeight")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    internal enum CoreFontWeight
    {
        [NativeValueName("WeightThin")]
        Thin = 100,
        [NativeValueName("WeightExtraLight")]
        ExtraLight = 200,
        [NativeValueName("WeightLight")]
        Light = 300,
        [NativeValueName("WeightSemiLight")]
        SemiLight = 350,
        [NativeValueName("WeightNormal")]
        Normal = 400,
        [NativeValueName("WeightMedium")]
        Medium = 500,
        [NativeValueName("WeightSemiBold")]
        SemiBold = 600,
        [NativeValueName("WeightBold")]
        Bold = 700,
        [NativeValueName("WeightExtraBold")]
        ExtraBold = 800,
        [NativeValueName("WeightBlack")]
        Black = 900,
        [NativeValueName("WeightExtraBlack")]
        ExtraBlack = 950,
    }

    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [TypeFlags(IsWebHostHidden = false)]
    [DXamlModifier(Modifier.Public)]
    [NativeName("FontStyle")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum FontStyle
    {
        [NativeValueName("StyleNormal")]
        Normal = 0,
        [NativeValueName("StyleOblique")]
        Oblique = 1,
        [NativeValueName("StyleItalic")]
        Italic = 2,
    }

    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [TypeFlags(IsWebHostHidden = false)]
    [DXamlModifier(Modifier.Public)]
    [NativeName("FontStretch")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum FontStretch
    {
        [NativeValueName("StretchUndefined")]
        Undefined = 0,
        [NativeValueName("StretchUltraCondensed")]
        UltraCondensed = 1,
        [NativeValueName("StretchExtraCondensed")]
        ExtraCondensed = 2,
        [NativeValueName("StretchCondensed")]
        Condensed = 3,
        [NativeValueName("StretchSemiCondensed")]
        SemiCondensed = 4,
        [NativeValueName("StretchNormal")]
        Normal = 5,
        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeValueName("StretchMedium")]
        Medium = 5,
        [NativeValueName("StretchSemiExpanded")]
        SemiExpanded = 6,
        [NativeValueName("StretchExpanded")]
        Expanded = 7,
        [NativeValueName("StretchExtraExpanded")]
        ExtraExpanded = 8,
        [NativeValueName("StretchUltraExpanded")]
        UltraExpanded = 9,
    }

    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [NativeName("TextDecorations")]
    [Platform(typeof(Windows.Foundation.UniversalApiContract), 4)]
    [EnumFlags(AreValuesFlags = true, HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("This property determines if text should have underline or strikethrough")]
    [DXamlIdlGroup("coretypes2")]
    public enum TextDecorations
    {
        [NativeComment("Text is with no underline/strikethrough")]
        [NativeValueName("TextDecorationsNone")]
        None = 0,
        [NativeComment("Text is with underline")]
        [NativeValueName("TextDecorationsUnderline")]
        Underline = 1,
        [NativeComment("Text is with strikethrough")]
        [NativeValueName("TextDecorationsStrikethrough")]
        Strikethrough = 2,
    }
}

namespace Microsoft.UI.Text
{
    [Imported]
    [TypeTable(ForceInclude = true)]
    [WindowsTypePattern]
    public interface ITextDocument
    {
    }

    [Imported]
    [DefaultInterfaceName("ITextDocument")]
    [WindowsTypePattern]
    public class RichEditTextDocument
    {
    }
}
