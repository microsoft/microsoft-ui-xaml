// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal class ForwardDeclaringNamespace
    {
        public String Namespace { get; private set; }
        public List<String> ShortNameTypes { get; private set; }

        public ForwardDeclaringNamespace(String typePath, List<String> typeNames)
        {
            Namespace = typePath;
            ShortNameTypes = typeNames;
        }
    }
}
