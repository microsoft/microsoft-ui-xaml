// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace MS.Internal.Automation
{
    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextProvider")]
    [Guids(ClassGuid = "0c09cbb5-887a-46f1-bb42-7f2fa741959c")]
    public abstract class TextProvider
     : Microsoft.UI.Xaml.DependencyObject
    {
        protected TextProvider() {}
    }
    
    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextRangeProvider")]
    [Guids(ClassGuid = "03f69d3b-78b8-44dc-b72c-5c7e666d9c99")]
    public abstract class TextRangeProvider
     : Microsoft.UI.Xaml.DependencyObject
    {
        protected TextRangeProvider() {}
    }
    
}

