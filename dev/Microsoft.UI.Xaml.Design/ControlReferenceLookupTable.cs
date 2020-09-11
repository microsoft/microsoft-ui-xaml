// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Design
{
    class ControlReferenceLookupTable
    {
        const string WinUIRootNameSpace = "Microsoft.UI.Xaml.Control";
        public static string GetReference(string name)
        {
            return WinUIRootNameSpace + "." + name;
        }
    }
}
