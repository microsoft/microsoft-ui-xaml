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

    // Writes the roots log as XAML, the same way that serializing a Roots object with the XAML object reader and writer would (the
    // log is a file format of its own, that tools read): the default namespace of the classes above, one property element per
    // non-empty list, and the members of a type as its content. It is written by hand because the object reader needs reflection.
    static class RootsXmlWriter
    {
        public const string Namespace = "clr-namespace:Microsoft.UI.Xaml.Markup.Compiler.RootLog;assembly=Microsoft.UI.Xaml.Markup.Compiler";

        public static void Write(Roots roots, System.Xml.XmlWriter writer)
        {
            writer.WriteStartDocument();
            writer.WriteStartElement("Roots", Namespace);
            writer.WriteAttributeString("xmlns", Namespace);

            if (roots.Interfaces.Count > 0)
            {
                writer.WriteStartElement("Roots.Interfaces", Namespace);
                foreach (RootInterface rootInterface in roots.Interfaces)
                {
                    writer.WriteStartElement("RootInterface", Namespace);
                    writer.WriteAttributeString("FullName", EscapeValue(rootInterface.FullName));
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
            }

            if (roots.PropertyPathNames.Count > 0)
            {
                writer.WriteStartElement("Roots.PropertyPathNames", Namespace);
                foreach (RootPropertyPathName propertyPathName in roots.PropertyPathNames)
                {
                    writer.WriteStartElement("RootPropertyPathName", Namespace);
                    writer.WriteAttributeString("Name", EscapeValue(propertyPathName.Name));
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
            }

            if (roots.RootTypes.Count > 0)
            {
                writer.WriteStartElement("Roots.RootTypes", Namespace);
                foreach (RootType rootType in roots.RootTypes)
                {
                    writer.WriteStartElement("RootType", Namespace);
                    writer.WriteAttributeString("FullName", EscapeValue(rootType.FullName));
                    foreach (RootMember member in rootType.Members)
                    {
                        writer.WriteStartElement(GetElementName(member), Namespace);
                        writer.WriteAttributeString("Name", EscapeValue(member.Name));
                        writer.WriteEndElement();
                    }
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
            }

            writer.WriteEndElement();
            writer.WriteEndDocument();
        }

        // A value that starts with '{' would be read back as a markup extension, so it is escaped as the XAML writer does
        private static string EscapeValue(string value)
        {
            return value != null && value.StartsWith("{", StringComparison.Ordinal) ? "{}" + value : value;
        }

        private static string GetElementName(RootMember member)
        {
            if (member is RootProperty)
            {
                return "RootProperty";
            }
            if (member is RootEvent)
            {
                return "RootEvent";
            }
            return "RootMethod";
        }
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
