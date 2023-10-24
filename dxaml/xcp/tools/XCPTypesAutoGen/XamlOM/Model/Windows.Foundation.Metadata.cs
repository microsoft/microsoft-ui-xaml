// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.Foundation.Metadata
{
    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [Platform(1, typeof(Windows.Foundation.UniversalApiContract), 3)]
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, AllowMultiple = false, Inherited = false)]
    [IdlAttributeTarget(AttributeTargets.Class | AttributeTargets.Struct)]
    public class CreateFromStringAttribute : Attribute
    {
        public string MethodName { get; set; }
    }
}