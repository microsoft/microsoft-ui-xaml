// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen.Templates.ModernIDL
{
    public static class ClassDefinitionExtensions
    {
        public static string GetRuntimeClassString(this OM.ClassDefinition def)
        {
            return def.IdlClassInfo.IsSealed ? "runtimeclass" : "unsealed runtimeclass";
        }
    }
}
