// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class EventDefinition : MemberDefinition
    {
        public static readonly EventDefinition UnknownEvent;

        public virtual string AddName
        {
            get
            {
                return "add_" + IdlEventInfo.Name;
            }
        }

        public string ControlEventNamePointer
        {
            get
            {
                if (!XamlEventFlags.IsControlEvent)
                {
                    throw new InvalidOperationException("This operation is only valid on control events.");
                }
                return "&CUIElement::On" + Name;
            }
        }

        public DelegateDefinition Delegate
        {
            get
            {
                return (DelegateDefinition)EventHandlerType.IdlInfo.Type;
            }
        }

        public ClassDefinition EventArgsType
        {
            get
            {
                return (ClassDefinition)Delegate.Parameters[1].ParameterType.IdlInfo.Type;
            }
        }

        public TypeReference EventHandlerType
        {
            get;
            set;
        }

        public ClassDefinition EventSenderType
        {
            get
            {
                return (ClassDefinition)Delegate.Parameters[0].ParameterType.IdlInfo.Type;
            }
        }

        public string EventSourceMemberName
        {
            get
            {
                return String.Format("m_{0}EventSource", Name);
            }
        }

        public string EventSourceTypeName
        {
            get
            {
                return IdlEventInfo.Name + "EventSourceType";
            }
        }

        public string GetEventSourceName
        {
            get
            {
                return string.Format("Get{0}EventSourceNoRef", IdlEventInfo.Name);
            }
        }

        public Idl.IdlEventInfo IdlEventInfo
        {
            get;
            private set;
        }

        public override Idl.IdlMemberInfo IdlMemberInfo
        {
            get
            {
                return IdlEventInfo;
            }
        }

        public string IndexName
        {
            get;
            set;
        }

        public string IndexNameWithCastToInt
        {
            get
            {
                return "static_cast<int>(" + IndexName + ")";
            }
        }

        public string IndexNameWithoutPrefix
        {
            get;
            set;
        }

        public bool IsEventSourcePureVirtual
        {
            get
            {
                return !GenerateDefaultBody;
            }
        }

        public bool IsRouted
        {
            get;
            set;
        }

        public virtual string RemoveName
        {
            get
            {
                return "remove_" + IdlEventInfo.Name;
            }
        }

        /// <summary>
        /// Gets whether the event should use the event manager.
        /// </summary>
        public bool UseEventManager
        {
            get
            {
                return XamlEventFlags.UseEventManager || XamlEventFlags.IsControlEvent;
            }
        }

        public string SimplePropertyChangedHandlerName
        {
            get
            {
                return "SimplePropertyChangedHandler_" + Name;
            }
        }

        public bool IsSimplePropertyChangedEvent
        {
            get
            {
                return SimplePropertyEventSourceMember != null;
            }
        }

        public DependencyPropertyDefinition SimplePropertyEventSourceMember
        {
            get
            {
                return DeclaringClass.SimpleProperties.FirstOrDefault(p => string.Compare(p.Name, SimplePropertyEventSourceMemberName) == 0);
            }
        }

        public string SimplePropertyEventSourceMemberName
        {
            get;
            set;
        }

        public XamlEventFlags XamlEventFlags
        {
            get;
            set;
        }

        public bool GenerateStableIndex { get; set; } = true;

        static EventDefinition()
        {
            UnknownEvent = new EventDefinition()
            {
                Name = "UnknownEvent",
                DeclaringType = ClassDefinition.UnknownType
            };
            UnknownEvent.IdlMemberInfo.IsExcluded = true;
        }

        public EventDefinition()
        {
            IdlEventInfo = new Idl.IdlEventInfo(this);
            XamlEventFlags = new XamlEventFlags();
        }

        private static TypeReference TokenType = new TypeReference(
            new ClassDefinition() { IsPrimitive = true, IsValueType = true, PrimitiveCppName = "EventRegistrationToken" })
        {
            Name = "token"
        };

        public virtual MethodDefinition GetAddMethod()
        {
            MethodDefinition addMethod = new MethodDefinition();
            Helper.ShallowCopyProperties<MemberDefinition>(this, addMethod);
            Helper.ShallowCopyProperties(IdlMemberInfo, addMethod.IdlMemberInfo);
            addMethod.Name = AddName;
            addMethod.IdlMethodInfo.Name = AddName;
            addMethod.Parameters.Add(new ParameterDefinition() { ParameterType = EventHandlerType });
            addMethod.IsImplVirtual = XamlEventFlags.IsImplVirtual;
            addMethod.ReturnType = TokenType;
            return addMethod;
        }

        public virtual MethodDefinition GetRemoveMethod()
        {
            MethodDefinition removeMethod = new MethodDefinition();
            Helper.ShallowCopyProperties<MemberDefinition>(this, removeMethod);
            Helper.ShallowCopyProperties(IdlMemberInfo, removeMethod.IdlMemberInfo);
            removeMethod.Name = RemoveName;
            removeMethod.IdlMethodInfo.Name = RemoveName;
            removeMethod.IsImplVirtual = XamlEventFlags.IsImplVirtual;
            removeMethod.Parameters.Add(new ParameterDefinition() { ParameterType = TokenType });
            return removeMethod;
        }
    }
}
