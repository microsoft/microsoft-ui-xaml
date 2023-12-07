// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Microsoft.UI.Xaml.Resources
{
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("coretypes2")]
    [Guids(ClassGuid = "55e4dbf3-2fb5-449f-b048-f4cbda22efe6")]
    public class CustomXamlResourceLoader
     : Windows.Foundation.Object
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public static Microsoft.UI.Xaml.Resources.CustomXamlResourceLoader Current
        {
            get;
            set;
        }

        public CustomXamlResourceLoader() {}

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Windows.Foundation.Object GetResource(Windows.Foundation.String resourceId, Windows.Foundation.String objectType, Windows.Foundation.String propertyName, Windows.Foundation.String propertyType)
        {
            return default(Windows.Foundation.Object);
        }
    }
    
}

