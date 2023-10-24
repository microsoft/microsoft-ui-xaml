// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Visitors
{
    public abstract class OMVisitor
    {
        private OMContextView _view;
        private HashSet<TypeDefinition> _visitedTypes;

        protected OMVisitor(OMContextView view)
        {
            _view = view;
        }

        protected void Run()
        {
            _visitedTypes = new HashSet<TypeDefinition>();
            foreach (NamespaceDefinition ns in SortNamespaces(_view.Namespaces))
            {
                VisitNamespace(ns);
            }
            _visitedTypes = null;
        }

        protected virtual void VisitNamespace(NamespaceDefinition ns)
        {
            IEnumerable<TypeDefinition> allTypes = ns.Classes.OfType<TypeDefinition>()
                .Concat(ns.Enums.OfType<TypeDefinition>())
                .Concat(ns.Delegates.OfType<TypeDefinition>())
                .Concat(ns.Attributes.OfType<TypeDefinition>());
            allTypes = SortTypes(allTypes);

            foreach (TypeDefinition type in allTypes)
            {
                VisitTypeInternal(type);
            }
        }

        protected virtual IEnumerable<NamespaceDefinition> SortNamespaces(IEnumerable<NamespaceDefinition> namespaces)
        {
            return namespaces;
        }

        protected virtual IEnumerable<TypeDefinition> SortTypes(IEnumerable<TypeDefinition> allTypes)
        {
            return allTypes;
        }

        protected virtual void VisitDelegate(DelegateDefinition d)
        {
            foreach (ParameterDefinition p in d.Parameters)
            {
                VisitParameter(p);
            }

            VisitReturnType(d.ReturnType);
        }

        protected virtual void VisitAttribute(AttributeDefinition a)
        {
            foreach (PropertyDefinition p in a.Properties)
            {
                VisitAttributeProperty(p);
            }
        }

        protected virtual void VisitAttributeProperty(PropertyDefinition p)
        {
            VisitProperty(p);
        }

        protected virtual void VisitReturnType(TypeReference typeRef)
        {
            if (typeRef != null)
            {
                VisitTypeRef(typeRef);
            }
        }

        protected void VisitTypeInternal(TypeDefinition type)
        {
            if (_visitedTypes.Contains(type))
            {
                return;
            }

            _visitedTypes.Add(type);
            VisitType(type);
        }

        protected virtual void VisitType(TypeDefinition type)
        {
            if (type is ClassDefinition)
            {
                VisitClass((ClassDefinition)type);
            }
            else if (type is EnumDefinition)
            {
                VisitEnum((EnumDefinition)type);
            }
            else if (type is DelegateDefinition)
            {
                VisitDelegate((DelegateDefinition)type);
            }
            else if (type is AttributeDefinition)
            {
                VisitAttribute((AttributeDefinition)type);
            }
        }

        protected virtual void VisitEnum(EnumDefinition e)
        {
            foreach (EnumValueDefinition enumValue in e.Values)
            {
                VisitEnumValue(enumValue);
            }
        }

        protected virtual void VisitEnumValue(EnumValueDefinition enumValue)
        {
        }

        protected virtual void VisitClass(ClassDefinition c)
        {
            foreach (PropertyDefinition p in c.Properties)
            {
                VisitMember(p);
            }

            foreach (EventDefinition e in c.Events)
            {
                VisitMember(e);
            }

            foreach (MethodDefinition m in c.Methods)
            {
                VisitMember(m);
            }

            if (c.BaseClass != null)
            {
                VisitTypeInternal(c.BaseClass);
            }

            foreach (TypeReference implementedInterface in c.ImplementedInterfaces)
            {
                VisitImplementedInterface(implementedInterface);
            }
        }

        protected virtual void VisitImplementedInterface(TypeReference implementedInterface)
        {
            VisitTypeRef(implementedInterface);
        }

        protected virtual void VisitMember(MemberDefinition m)
        {
            if (m is PropertyDefinition)
            {
                VisitProperty((PropertyDefinition)m);
            }
            else if (m is EventDefinition)
            {
                VisitEvent((EventDefinition)m);
            }
            else if (m is MethodBaseDefinition)
            {
                VisitMethodBase((MethodBaseDefinition)m);
            }
        }

        protected virtual void VisitMethodBase(MethodBaseDefinition m)
        {
            if (m is ConstructorDefinition)
            {
                VisitConstructor((ConstructorDefinition)m);
            }
            else if (m is MethodDefinition)
            {
                VisitMethod((MethodDefinition)m);
            }

            foreach (ParameterDefinition p in m.Parameters)
            {
                VisitParameter(p);
            }
        }

        protected virtual void VisitParameter(ParameterDefinition p)
        {
            VisitTypeRef(p.ParameterType);
        }

        protected virtual void VisitConstructor(ConstructorDefinition c)
        {
        }

        protected virtual void VisitProperty(PropertyDefinition p)
        {
            VisitTypeRef(p.PropertyType);
        }

        protected virtual void VisitEvent(EventDefinition e)
        {
            VisitTypeRef(e.EventHandlerType);
        }

        protected virtual void VisitMethod(MethodDefinition m)
        {
            VisitReturnType(m.ReturnType);
        }

        protected virtual void VisitTypeRef(TypeReference typeRef)
        {
            if (typeRef.Type != null)
            {
                VisitTypeInternal(typeRef.Type);
            }

            if (typeRef.IdlInfo.Type != null)
            {
                VisitTypeInternal(typeRef.IdlInfo.Type);
            }
        }
    }
}
