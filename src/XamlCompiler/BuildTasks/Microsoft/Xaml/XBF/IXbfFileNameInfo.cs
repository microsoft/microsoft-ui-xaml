// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    interface IXbfFileNameInfo
    {
        string GivenXamlName { get; set; }
        string InputXamlName { get; set; }
        string OutputXbfName { get; set; }
        string XamlFileChecksum { get; set; }
    }
}
