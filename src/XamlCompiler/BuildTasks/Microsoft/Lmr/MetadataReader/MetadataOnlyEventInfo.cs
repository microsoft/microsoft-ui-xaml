// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Implement an EventInfo based off System.Reflection.Metadata. 
    /// </summary>
    internal class MetadataOnlyEventInfo : EventInfo
    {
        public MetadataOnlyEventInfo(MetadataOnlyModule resolver, EventDefinitionHandle eventHandle, Type[] typeArgs, Type[] methodArgs)
        {
            m_resolver = resolver;
            m_eventDefHandle = eventHandle;
            m_context = new GenericContext(typeArgs, methodArgs);

            var eventDef = resolver.RawReader.GetEventDefinition(eventHandle);
            m_attrib = (EventAttributes)eventDef.Attributes;
            m_eventHandlerTypeHandle = eventDef.Type;

            var accessors = eventDef.GetAccessors();
            m_addMethodHandle = accessors.Adder;
            m_removeMethodHandle = accessors.Remover;
            m_raiseMethodHandle = accessors.Raiser;
        }

        public override string ToString()
        {
            return DeclaringType.ToString() + "." + Name;
        }

        /// <summary>
        /// Lookup event name only when really needed and cache it in this instance.
        /// </summary>
        private void InitializeName()
        {
            if (string.IsNullOrEmpty(m_name))
            {
                var eventDef = m_resolver.RawReader.GetEventDefinition(m_eventDefHandle);
                m_name = m_resolver.RawReader.GetString(eventDef.Name);
            }
        }

        #region EventInfo Members

        public override System.Reflection.EventAttributes Attributes
        {
            get { return m_attrib; }
        }

        public override MemberTypes MemberType
        {
            get
            {
                return MemberTypes.Event;
            }
        }

        public override string Name
        {
            get { InitializeName();  return m_name; }
        }

        public override object[] GetCustomAttributes(bool inherit)
        {
            throw new NotSupportedException();
        }

        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            throw new NotSupportedException();
        }

        public override bool IsDefined(Type attributeType, bool inherit)
        {
            throw new NotSupportedException();
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override Type ReflectedType
        {
            get { throw new NotSupportedException(); }
        }

        public override Type EventHandlerType
        {
            get
            {
                Type eventHandlerType = m_resolver.GetGenericType(m_eventHandlerTypeHandle, m_context);
                Debug.Assert(eventHandlerType != null);
                return eventHandlerType;
            }
        }

        public override Type DeclaringType
        {
            get
            {
                var typeDef = m_resolver.GetDeclaringTypeForEvent(m_eventDefHandle);
                Type declaringType = m_resolver.GetGenericType((EntityHandle)typeDef, m_context);
                Debug.Assert(declaringType != null);
                return declaringType;
            }
        }

        public override int MetadataToken 
        { 
            get 
            { 
                return MetadataTokens.GetToken(m_eventDefHandle); 
            } 
        }

        public override MethodInfo GetAddMethod(bool nonPublic)
        {
            if (m_addMethodHandle.IsNil)
            {
                return null;
            }
            MethodInfo addMethod = m_resolver.GetGenericMethodInfo(m_addMethodHandle, this.m_context);
            if (nonPublic || addMethod.IsPublic)
            {
                return addMethod;
            }
            return null;
        }

        public override MethodInfo GetRemoveMethod(bool nonPublic)
        {
            if (m_removeMethodHandle.IsNil)
            {
                return null;
            }
            MethodInfo removeMethod = m_resolver.GetGenericMethodInfo(m_removeMethodHandle, this.m_context);
            if (nonPublic || removeMethod.IsPublic)
            {
                return removeMethod;
            }
            return null;
        }

        public override MethodInfo GetRaiseMethod(bool nonPublic)
        {
            if (m_raiseMethodHandle.IsNil)
            {
                return null;
            }
            MethodInfo raiseMethod = m_resolver.GetGenericMethodInfo(m_raiseMethodHandle, this.m_context);
            if (nonPublic || raiseMethod.IsPublic)
            {
                return raiseMethod;
            }
            return null;
        }

        #endregion

        public override Module Module
        {
            get { return m_resolver; }
        }

        public override bool Equals(object obj)
        {
            MetadataOnlyEventInfo ev = obj as MetadataOnlyEventInfo;
            if (ev != null)
            {
                return ev.m_resolver.Equals(m_resolver) && (ev.m_eventDefHandle.Equals(m_eventDefHandle)) &&
                    (DeclaringType.Equals(ev.DeclaringType));
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return m_resolver.GetHashCode() * 32767 + m_eventDefHandle.GetHashCode();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return m_resolver.GetCustomAttributeData((EntityHandle)m_eventDefHandle);
        }

        private readonly MetadataOnlyModule m_resolver;
        private readonly EventDefinitionHandle m_eventDefHandle;
        private readonly EventAttributes m_attrib;
        private readonly EntityHandle m_eventHandlerTypeHandle;
        private readonly GenericContext m_context;
        private string m_name;
        private readonly MethodDefinitionHandle m_addMethodHandle;
        private readonly MethodDefinitionHandle m_removeMethodHandle;
        private readonly MethodDefinitionHandle m_raiseMethodHandle;
    }
}
