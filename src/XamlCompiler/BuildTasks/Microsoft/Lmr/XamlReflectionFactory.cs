// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using Microsoft.UI.Xaml.Markup.Compiler.Lmr;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    internal class XamlReflectionFactory : DefaultFactory
    {
        Dictionary<String, Dictionary<int, MetadataOnlyCommonType>> _scopeCache = new Dictionary<string, Dictionary<int, MetadataOnlyCommonType>>();

        // This is simply a performance check. Need to measure speed and consider adding other methods.
        public override MetadataOnlyCommonType CreateSimpleType(MetadataOnlyModule scope, TypeDefinitionHandle typeDefHandle)
        {
            MetadataOnlyCommonType type;
            Dictionary<int, MetadataOnlyCommonType> typeCache;

            if (!_scopeCache.TryGetValue(scope.FullyQualifiedName, out typeCache))
            {
                typeCache = new Dictionary<int, MetadataOnlyCommonType>();
                _scopeCache.Add(scope.FullyQualifiedName, typeCache);
            }

            int tokenValue = MetadataTokens.GetToken(typeDefHandle);
            if (!typeCache.TryGetValue(tokenValue, out type))
            {
                type = base.CreateSimpleType(scope, typeDefHandle);
                typeCache.Add(tokenValue, type);
            }
            return type;
        }
    }
}
