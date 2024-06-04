// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Prevents generation of value in EnumDefs.g.h (UIAEnums.g.h, etc…) and JoltClasses.g.cs.
    /// Example: Ieee/PAL_SOUND_WAVEFORMAT_IEEE
    /// </summary>
    [AttributeUsage(AttributeTargets.Field, Inherited = false)]
    public class NativeOnlyAttribute : Attribute
    {
    }
}
