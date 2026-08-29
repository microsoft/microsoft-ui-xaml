// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that a type is a custom control.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    public class ControlPatternAttribute : FrameworkTypePatternAttribute
    {
    }
}
