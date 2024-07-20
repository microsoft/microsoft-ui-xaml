// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies whether the type is currently written by hand.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Enum, Inherited = false)]
    public class HandWrittenAttribute : Attribute, NewBuilders.IEndClassBuilder
    {
        /// <summary>
        /// Gets or sets whether the type is currently written by hand in the core.
        /// </summary>
        public bool InCore
        {
            get;
            set;
        }

        public bool InFramework
        {
            get;
            set;
        }

        public HandWrittenAttribute()
        {
            InCore = true;
            InFramework = true;
        }

        public void BuildEndClass(OM.ClassDefinition definition, Type source)
        {
            definition.GenerateInCore = !InCore;
            definition.GenerateInFramework = !InFramework;
        }
    }
}
