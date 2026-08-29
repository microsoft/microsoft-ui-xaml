// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Microsoft.Internal.FrameworkUdk
{
    [Imported]
    [ClassFlags(IsHiddenFromIdl = true)]
    [Platform(typeof(Microsoft.UI.Xaml.PrivateApiContract), 1)]
    public interface IBackButtonPressedListener
    {
        Windows.Foundation.Boolean OnBackButtonPressed();
    }
}