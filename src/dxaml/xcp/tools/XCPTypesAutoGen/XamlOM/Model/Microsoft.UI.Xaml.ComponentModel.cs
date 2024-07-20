// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Microsoft.UI.Xaml.ComponentModel
{
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true)]
    [HandWritten]
    internal class XamlServiceProviderContext
    {

    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true)]
    [Guids(ClassGuid = "a5fbb92f-296e-4b34-92eb-c06891320908")]
    internal interface ISupportInitialize
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void BeginInit();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void EndInit([Optional] XamlServiceProviderContext context);
    }
    
}
