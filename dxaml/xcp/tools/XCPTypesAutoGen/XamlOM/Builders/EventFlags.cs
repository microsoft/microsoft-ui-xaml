// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies low-level flags that control the behavior of this event.
    /// </summary>
    [AttributeUsage(AttributeTargets.Event, Inherited = false)]
    public class EventFlagsAttribute : Attribute, NewBuilders.IEventBuilder
    {
        /// <summary>
        /// Generates event in UIElement.g.cpp, an event ID between PROPERTYINDEX_CONTROL_DELEGATE_FIRST 
        /// and PROPERTYINDEX_CONTROL_DELEGATE_LAST in Known.g.h, an entry in 
        /// 'private static readonly ControlDelegate[] _controlDelegates' in JoltClasses.g.cs.
        /// Examples: PROPERTYINDEX_UIELEMENT_MOUSEMOVE to PROPERTYINDEX_UIELEMENT_DROP
        /// </summary>
        public bool IsControlEvent { get; set; }

        /// <summary>
        /// Generates property flag PROP_HIDDEN in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_UIELEMENT_TEXTINPUT_END
        /// </summary>
        public bool IsHidden { get; set; }

        /// <summary>
        /// Gets or sets whether the event should use the event manager.
        /// </summary>
        public bool UseEventManager { get; set; }

        /// <summary>
        /// Controls whether the event's add/remove methods are virtual
        /// </summary>
        public bool IsImplVirtual { get; set; }

        public void BuildNewEvent(OM.EventDefinition definition, EventInfo source)
        {
            if (IsHidden)
            {
                definition.IdlMemberInfo.IsExcluded = true;
            }
            NewBuilders.Helper.ApplyFlagProperties(this, definition.XamlEventFlags);
        }
    }
}
