// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies an order hint. This is required for new members to reduce the impact of a breaking change.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Event | AttributeTargets.Method, Inherited = false)]
    public class OrderHintAttribute : Attribute, NewBuilders.IMemberBuilder
    {
        public int Order
        {
            get;
            private set;
        }

        public OrderHintAttribute(int order)
        {
            Order = order;
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            definition.OrderHint = Order;
        }
    }
}
