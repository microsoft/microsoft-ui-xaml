// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.Xaml.WidgetSpinner.Common
{
    internal static class Extensions
    {
        internal static bool XbfMetadataStringsAreNullTerminated(this Version version)
        {
            return version.Major == 2 && version.Minor >= 1;
        }

        internal static bool IsSupportedXbfVersion(this Version version)
        {
            return version.Major == 2 && version.Minor >= 0 && version.Minor <= 1;
        }

        internal static Stack<T> Clone<T>(this Stack<T> other)
        {
            return new Stack<T>(other.Reverse());
        }
    }
}