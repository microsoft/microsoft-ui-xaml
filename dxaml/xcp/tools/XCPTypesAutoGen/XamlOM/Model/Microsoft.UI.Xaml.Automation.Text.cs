// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Microsoft.UI.Xaml.Automation.Text
{
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, IsTypeConverter = true)]
    public enum TextUnit
    {
        Character = 0,
        Format = 1,
        Word = 2,
        Line = 3,
        Paragraph = 4,
        Page = 5,
        Document = 6,
    }
    
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, IsTypeConverter = true)]
    public enum TextPatternRangeEndpoint
    {
        Start = 0,
        End = 1,
    }
    
}

