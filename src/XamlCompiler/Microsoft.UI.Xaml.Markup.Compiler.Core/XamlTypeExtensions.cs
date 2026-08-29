// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public static class XamlTypeExtensions
    {
        internal static bool IsFloat(this XamlType source)
        {
            return source.UnderlyingType.FullName == typeof(float).FullName;
        }

        internal static bool IsString(this XamlType source)
        {
            return source.UnderlyingType.FullName == typeof(String).FullName;
        }

        internal static bool IsIntegerIndexable(this XamlType source)
        {
            return (source.IsArray || source.IsCollection) && source.ItemType != null;
        }

        internal static bool IsStringIndexable(this XamlType source)
        {
            return source.IsDictionary && source.ItemType != null;
        }

        public static bool ImplementsINotifyPropertyChanged(this XamlType type)
        {
            if (type is IXamlTypeMeta meta)
            {
                return meta != null ? meta.ImplementsINotifyPropertyChanged : false;
            }
            throw new ArgumentException("ImplementsINotifyPropertyChanged: XamlType does not have metadata", "type");
        }

        public static bool ImplementsINotifyCollectionChanged(this XamlType type)
        {
            if (type is IXamlTypeMeta meta)
            {
                return meta != null ? meta.ImplementsINotifyCollectionChanged : false;
            }
            throw new ArgumentException("ImplementsINotifyCollectionChanged: XamlType does not have metadata", "type");
        }

        public static bool ImplementsIObservableVector(this XamlType type)
        {
            if (type is IXamlTypeMeta meta)
            {
                return meta != null ? meta.ImplementsIObservableVector : false;
            }
            throw new ArgumentException("ImplementsIObservableVector: XamlType does not have metadata", "type");
        }

        public static bool ImplementsIObservableMap(this XamlType type)
        {
            if (type is IXamlTypeMeta meta)
            {
                return meta != null ? meta.ImplementsIObservableMap : false;
            }
            throw new ArgumentException("ImplementsIObservableMap: XamlType does not have metadata", "type");
        }

        public static bool ImplementsINotifyDataErrorInfo(this XamlType type)
        {
            if (type is IXamlTypeMeta meta)
            {
                return meta != null ? meta.ImplementsINotifyDataErrorInfo : false;
            }
            throw new ArgumentException("ImplementsINotifyDataErrorInfo: XamlType does not have metadata", "type");
        }

        private static bool InheritsFromNamedType(this Type type, string inheritedTypeName)
        {
            Type curType = type;
            while (curType != null && curType != curType.BaseType)
            {
                if (curType.FullName.Equals(inheritedTypeName))
                {
                    return true;
                }

                curType = curType.BaseType;
            }

            return false;
        }

        public static bool IsDependencyProperty(this Type declaringType, string propertyName)
        {
            var dpName = propertyName + "Property";
            var bindingFlags = BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.FlattenHierarchy;
            const string dependencyObjectTypeName = KnownTypes.DependencyObject;
            const string dependencyPropertyTypeName = KnownTypes.DependencyProperty;

            // Gotta be on DO, because that's where we register for notifications.
            if (declaringType.InheritsFromNamedType(dependencyObjectTypeName))
            {
                // They are properties in CX/C++ winmds.
                PropertyInfo pi = declaringType.GetProperty(dpName, bindingFlags);
                if (pi != null)
                {
                    return pi.PropertyType.InheritsFromNamedType(dependencyPropertyTypeName);
                }

                // In managed they may also be fields.
                FieldInfo fi = declaringType.GetField(dpName, bindingFlags);
                if (fi != null)
                {
                    return fi.FieldType.InheritsFromNamedType(dependencyPropertyTypeName);
                }
            }

            return false;
        }
    }
}
