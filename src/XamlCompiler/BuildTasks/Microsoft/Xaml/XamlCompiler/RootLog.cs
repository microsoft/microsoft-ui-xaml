// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Windows.Markup;

namespace Microsoft.UI.Xaml.Markup.Compiler.RootLog
{
    using DirectUI;

    class RootMember
    {
        public String Name { get; set; }
    }

    class RootProperty : RootMember
    {
    }

    class RootEvent : RootMember
    {
    }

    class RootMethod : RootMember
    {
    }

    class RootPropertyPathName
    {
        public String Name { get; set; }
    }

    class RootInterface
    {
        public String FullName { get; set; }
    }

    [ContentProperty("Members")]
    class RootType
    {
        public String FullName { get; set; }
        public List<RootMember> Members { get; private set; }

        public RootType()
        {
            Members = new List<RootMember>();
        }
    }

    class Roots
    {
        public Roots()
        {
            RootTypes = new List<RootType>();
            PropertyPathNames = new List<RootPropertyPathName>();
            Interfaces = new List<RootInterface>();
        }

        public List<RootType> RootTypes { get; private set; }
        public List<RootPropertyPathName> PropertyPathNames { get; private set; }
        public List<RootInterface> Interfaces { get; private set; }
    }

    // ------------------- Builders -------------------

    class RootLogBuilder
    {
        Dictionary<String, RootTypeBuilder> _typeDict = new Dictionary<string, RootTypeBuilder>();
        Dictionary<String, RootPropertyPathName> _propertyPathNameDict = new Dictionary<string, RootPropertyPathName>();
        Dictionary<String, RootInterface> _interfaceDict = new Dictionary<string, RootInterface>();
        bool _haveEntered_IValueConverter = false;
        bool _haveEntered_IXamlMetadataProvider = false;

        public Roots GetRoots()
        {
            Roots roots = new Roots();
            foreach (RootTypeBuilder typeBuilder in _typeDict.Values)
            {
                roots.RootTypes.Add(typeBuilder.GetRootType());
            }
            roots.Interfaces.AddRange(_interfaceDict.Values);
            roots.PropertyPathNames.AddRange(_propertyPathNameDict.Values);

            return roots;
        }

        public RootInterface AddInterface(string name)
        {
            RootInterface interfaceName;
            if (!_interfaceDict.TryGetValue(name, out interfaceName))
            {
                interfaceName = new RootInterface() { FullName = name };
                _interfaceDict.Add(name, interfaceName);
            }
            return interfaceName;
        }

        public RootPropertyPathName AddPropertyPathName(string name)
        {
            RootPropertyPathName propertyPathName;
            if (!_propertyPathNameDict.TryGetValue(name, out propertyPathName))
            {
                propertyPathName = new RootPropertyPathName() { Name = name };
                _propertyPathNameDict.Add(name, propertyPathName);
            }
            return propertyPathName;
        }

        public RootTypeBuilder AddTypeBuilder(DirectUIXamlType duiType)
        {
            CheckTypeForInterfaces(duiType);
            string standardName = duiType.UnderlyingType.FullName;
            RootTypeBuilder rType;
            if (!_typeDict.TryGetValue(standardName, out rType))
            {
                rType = new RootTypeBuilder(standardName);
                _typeDict.Add(standardName, rType);
            }
            return rType;
        }

        public RootMember AddProperty(DirectUIXamlType duiType, String name)
        {
            RootTypeBuilder rType = AddTypeBuilder(duiType);
            return rType.AddProperty(name);
        }

        public RootMember AddEvent(DirectUIXamlType duiType, String name)
        {
            RootTypeBuilder rType = AddTypeBuilder(duiType);
            return rType.AddEvent(name);
        }

        private void CheckTypeForInterfaces(DirectUIXamlType duiType)
        {
            if (!_haveEntered_IValueConverter)
            {
                if (duiType.IsValueConverter)
                {
                    AddInterface(KnownTypes.IValueConverter);
                    _haveEntered_IValueConverter = true;
                }
            }
            if (!_haveEntered_IXamlMetadataProvider)
            {
                if (duiType.IsMetadataProvider)
                {
                    AddInterface(KnownTypes.IXamlMetadataProvider);
                    _haveEntered_IXamlMetadataProvider = true;
                }
            }
        }
    }

    class RootTypeBuilder
    {
        Dictionary<String, RootProperty> _propertyDict = new Dictionary<string, RootProperty>();
        Dictionary<String, RootMethod> _MethodDict = new Dictionary<string, RootMethod>();
        Dictionary<String, RootEvent> _EventDict = new Dictionary<string, RootEvent>();

        public RootTypeBuilder(string standardName)
        {
            FullName = standardName;
        }

        public String FullName { get; private set; }

        public RootMember AddProperty(String name)
        {
            RootProperty rProperty;
            if (!_propertyDict.TryGetValue(name, out rProperty))
            {
                rProperty = new RootProperty() { Name = name };
                _propertyDict.Add(name, rProperty);
            }
            return rProperty;
        }

        public RootMember AddEvent(String name)
        {
            RootEvent rEvent;
            if (!_EventDict.TryGetValue(name, out rEvent))
            {
                rEvent = new RootEvent() { Name = name };
                _EventDict.Add(name, rEvent);
            }
            return rEvent;
        }

        public RootMember AddMethod(String name)
        {
            RootMethod rMethod;
            if (!_MethodDict.TryGetValue(name, out rMethod))
            {
                rMethod = new RootMethod() { Name = name };
                _MethodDict.Add(name, rMethod);
            }
            return rMethod;
        }

        public RootType GetRootType()
        {
            RootType rType = new RootType();

            rType.FullName = FullName;
            rType.Members.AddRange(_propertyDict.Values);
            rType.Members.AddRange(_EventDict.Values);
            rType.Members.AddRange(_MethodDict.Values);

            return rType;
        }
    }
}
