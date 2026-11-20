// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Diagnostics.CodeAnalysis;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Where <see cref="LmrReflection"/> sends the reflection calls on runtime types (as opposed to LMR types, which
    /// are read from metadata).
    /// </summary>
    /// <remarks>
    /// Every type the compiler reflects over is an LMR type, except the built-in types of the XAML language (see
    /// <c>XamlLanguage</c>), which are created from real runtime types. Those are the only runtime types that go through
    /// here, and this is the one place to decide what a build of the compiler does with them: this build lets any runtime
    /// type through, and a build that has to know statically which runtime types are reflected over restricts them here.
    /// </remarks>
    internal static class KnownRuntimeTypes
    {
        /// <summary>
        /// Gets the runtime type to call the reflection member <paramref name="member"/> on.
        /// </summary>
        [return: DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.All)]
        public static Type Get(Type type, string member)
        {
            return type;
        }
    }
}
