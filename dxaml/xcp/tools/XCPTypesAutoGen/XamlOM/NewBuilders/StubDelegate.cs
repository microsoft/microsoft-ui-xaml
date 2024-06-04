// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the delegate is a stub. The real delegate is either a Windows.Foundation.EventHandler or Windows.Foundation.TypedEventHandler.
    /// This attribute should be removed once old code-gen is gone.
    /// </summary>
    [AttributeUsage(AttributeTargets.Delegate, Inherited = false)]
    public class StubDelegateAttribute : Attribute, NewBuilders.IDelegateBuilder
    {
        public StubDelegateAttribute()
        {
        }

        public void BuildNewDelegate(OM.DelegateDefinition definition, Type source)
        {
            definition.IdlTypeInfo.IsExcluded = true;
        }
    }
}
