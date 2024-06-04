// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the kind of event-handler to generate in IDL.
    /// </summary>
    [AttributeUsage(AttributeTargets.Event, Inherited = false)]
    public class EventHandlerTypeAttribute : Attribute, NewBuilders.IEventBuilder
    {
        public EventHandlerKind Kind
        {
            get;
            private set;
        }

        public EventHandlerTypeAttribute(EventHandlerKind kind)
        {
            Kind = kind;
        }

        public void BuildNewEvent(OM.EventDefinition definition, EventInfo source)
        {
            // Until we update XamlOM/Model/* to use TypedEventHandler<>, etc, fix things up for the new code-gen here.
            OM.DelegateDefinition oldDelegateType = (OM.DelegateDefinition)definition.EventHandlerType.Type;

            switch (Kind)
            {
                case EventHandlerKind.TypedArgs:
                    definition.EventHandlerType = NewBuilders.Helper.GetEventHandlerTypeRef(typeof(Windows.Foundation.EventHandler<>).MakeGenericType(NewBuilders.Helper.GetClrType(oldDelegateType.Parameters[1].ParameterType.Type)));
                    break;
                case EventHandlerKind.TypedSenderAndArgs:
                    definition.EventHandlerType = NewBuilders.Helper.GetEventHandlerTypeRef(typeof(Windows.Foundation.TypedEventHandler<,>).MakeGenericType(
                        NewBuilders.Helper.GetClrType(definition.DeclaringType),
                        NewBuilders.Helper.GetClrType(oldDelegateType.Parameters[1].ParameterType.Type)));
                    break;
            }
        }
    }
}
