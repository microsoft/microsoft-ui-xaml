// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Peels SignatureModifiedType and SignaturePinnedType wrappers off a Type
    /// produced by LmrTypeProvider, recovering the bare Type + accumulated
    /// custom modifiers + pinned flag into a TypeSignatureDescriptor.
    /// </summary>
    internal static class SignatureUnwrap
    {
        /// <summary>
        /// Unwrap a Type that may be wrapped in SignatureModifiedType / SignaturePinnedType layers.
        /// Returns the bare Type, accumulated modifiers, and pinned flag.
        /// </summary>
        internal static TypeSignatureDescriptor Unwrap(Type t)
        {
            var optionalModifiers = new List<Type>();
            var requiredModifiers = new List<Type>();
            bool pinned = false;

            while (true)
            {
                if (t is SignaturePinnedType pinnedType)
                {
                    pinned = true;
                    t = pinnedType.Underlying;
                    continue;
                }

                if (t is SignatureModifiedType modifiedType)
                {
                    if (modifiedType.IsRequired)
                    {
                        requiredModifiers.Add(modifiedType.Modifier);
                    }
                    else
                    {
                        optionalModifiers.Add(modifiedType.Modifier);
                    }
                    t = modifiedType.Underlying;
                    continue;
                }

                break;
            }

            return new TypeSignatureDescriptor
            {
                Type = t,
                CustomModifiers = new CustomModifiers(optionalModifiers, requiredModifiers),
                IsPinned = pinned
            };
        }
    }
}
