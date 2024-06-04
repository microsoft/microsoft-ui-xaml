// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlEventInfo : IdlMemberInfo
    {
        private EventDefinition _owner;

        public bool ForwardDeclareIReference
        {
            get
            {
                return _owner.Modifier == Modifier.Public // Hide all non-public
                    && !_owner.IdlMemberInfo.IsExcluded // Hide all excluded idl members
                    && !IsExcluded // Hide all excluded idl events
                    && !_owner.XamlEventFlags.IsHidden // Hide all hidden events
                    && !_owner.XamlEventFlags.UseEventManager // Hide all core events
                    && string.IsNullOrWhiteSpace(_owner.VelocityFeatureName) // Hide all velocity events
                    && (_owner.DeclaringClass.IsAUIElement || (_owner is OM.AttachedEventDefinition && ((OM.AttachedEventDefinition)_owner).TargetType.Type.IsAUIElement)) // Restrict to classes related to UIElement
                    && _owner.GenerateStub; // Restrict to events that are created by codegen
            }
        }

        internal IdlEventInfo(EventDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
