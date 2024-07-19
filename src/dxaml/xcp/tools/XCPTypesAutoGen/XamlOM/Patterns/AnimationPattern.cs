// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the type represents a kind of animation.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class AnimationPatternAttribute : FrameworkTypePatternAttribute
    {
    }
}
