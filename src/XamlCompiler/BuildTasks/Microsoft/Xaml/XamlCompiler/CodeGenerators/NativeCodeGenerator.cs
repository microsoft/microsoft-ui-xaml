// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal abstract class NativeCodeGenerator<T> : CodeGenerator<T>
    {
        public static String Colonize(string typeName)
        {
            return KnownStrings.Colonize(typeName);
        }

        public static string Globalize(string fullType)
        {
            return $"::{Colonize(fullType).TrimStart(':')}";
        }

    }
}
