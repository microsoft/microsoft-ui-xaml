// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

// The namespace is prefixed with "MicrosoftWindows" instead of "Microsoft.Windows"
// in order to avoid C# namespace conflicts with types imported from the "Windows."
// namespace that are used elsewhere in the codegen. The `CustomNames` attribute 
// should be declared on each type belonging to this alternate namespace so that the 
// generated IDL is correct.
namespace MicrosoftWindows.ApplicationModel.Resources
{
    [Imported("Microsoft.Windows.ApplicationModel.Resources.idl")]
    [WindowsTypePattern]
    [CustomNames(RealTypeName = "Microsoft.Windows.ApplicationModel.Resources.IResourceManager")]
    public interface IResourceManager
    {
    }
}