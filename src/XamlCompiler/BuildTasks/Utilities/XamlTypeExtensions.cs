// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using DirectUI;
    using XamlDom;
    using Properties;

    internal static class MoreXamlTypeExtensions
    {
        private static IDictionary<String, Dictionary<String, bool>> implicitCastCache = new Core.InstanceCache<string, Dictionary<String, bool>>();
        private static IReadOnlyDictionary<String, HashSet<String>> primitiveTypesCasting = new Dictionary<String, HashSet<String>>()
            {
                // Boolean, Char, IntPtr, UIntPtr, Int64, UInt64 and Double can't convert to anything else.
                { typeof(SByte).FullName,  new HashSet<String>() { typeof(Int16).FullName, typeof(Int32).FullName, typeof(Int64).FullName, typeof(Single).FullName, typeof(Double).FullName } },
                { typeof(Byte).FullName,   new HashSet<String>() { typeof(Int16).FullName, typeof(UInt16).FullName, typeof(Int32).FullName, typeof(UInt32).FullName, typeof(Int64).FullName, typeof(UInt64).FullName, typeof(Single).FullName, typeof(Double).FullName } },
                { typeof(Int16).FullName,  new HashSet<String>() { typeof(Int32).FullName, typeof(Int64).FullName, typeof(Single).FullName, typeof(Double).FullName } },
                { typeof(UInt16).FullName, new HashSet<String>() { typeof(Int32).FullName, typeof(UInt32).FullName, typeof(Int64).FullName, typeof(UInt64).FullName, typeof(Single).FullName, typeof(Double).FullName } },
                { typeof(Int32).FullName,  new HashSet<String>() { typeof(Int64).FullName, typeof(Single).FullName, typeof(Double).FullName } },
                { typeof(UInt32).FullName, new HashSet<String>() { typeof(Int64).FullName, typeof(UInt64).FullName, typeof(Double).FullName } },
                { typeof(Single).FullName, new HashSet<String>() { typeof(Double).FullName } },
                { typeof(bool).FullName,   new HashSet<string>() { "Windows.Foundation.IReference`1<Boolean>" } },
            };

        #region InlineConversionLookupTable

        private static IDictionary<string, Dictionary<string, LanguageSpecificString>> inlineConversionsCache = new Core.InstanceCache<string, Dictionary<string, LanguageSpecificString>>();
        internal static IDictionary<string, Dictionary<string, LanguageSpecificString>> InlineConversionsCache
        {
            get
            {
                if (inlineConversionsCache.Count == 0)
                {
                    // Populate cache with supported inline conversions.
                    foreach (var key in inlineConversionsSupported)
                    {
                        inlineConversionsCache.Add(key.Key, key.Value);
                    }
                }
                return inlineConversionsCache;
            }
        }

        private static IReadOnlyDictionary<string, Dictionary<string, LanguageSpecificString>> inlineConversionsSupported = new Dictionary<string, Dictionary<string, LanguageSpecificString>>()
        {
            {
                typeof(bool).FullName, // from bool
                new Dictionary<string, LanguageSpecificString>()
                {
                    {
                        KnownTypes.Visibility, // to Visibility
                        new LanguageSpecificString(
                            () => $"{{0}} ? ::{KnownTypes.VisibilityColonized}::Visible : ::{KnownTypes.VisibilityColonized}::Collapsed",
                            () => $"{{0}} ? ::winrt::{KnownTypes.VisibilityColonized}::Visible : ::winrt::{KnownTypes.VisibilityColonized}::Collapsed",
                            () => $"{{0}} ? global::{KnownTypes.Visibility}.Visible : global::{KnownTypes.Visibility}.Collapsed",
                            () => $"If({{0}}, Global.{KnownTypes.Visibility}.Visible, Global.{KnownTypes.Visibility}.Collapsed)")
                    },
                    {
                        "Windows.Foundation.IReference`1<Boolean>", // to cx nullable bool
                        new LanguageSpecificString(() => "{0}")
                    },
                }
            },
            {
                "System.Nullable`1<Boolean>", // from bool? - XamlSchemaCodeInfo.GetFullGenericNestedName(typeof(bool?), true)
                new Dictionary<string, LanguageSpecificString>()
                {
                    {
                        KnownTypes.Visibility, // to visibility
                        new LanguageSpecificString(
                            () => $"{{0}} && {{0}}->Value ? ::{KnownTypes.VisibilityColonized}::Visible : ::{KnownTypes.VisibilityColonized}::Collapsed",
                            () => throw new NotImplementedException("Unexpected System.Nullable<Boolean> to Visibility"), // this should go through IReference<> (see below)
                            () => $"({{0}} ?? false) ? global::{KnownTypes.Visibility}.Visible : global::{KnownTypes.Visibility}.Collapsed",
                            () => $"If(If({{0}}, False), Global.{KnownTypes.Visibility}.Visible, Global.{KnownTypes.Visibility}.Collapsed)")
                    },
                }
            },
            {
                "Windows.Foundation.IReference`1<Boolean>", // from cx nullable bool
                new Dictionary<string, LanguageSpecificString>()
                {
                    {
                        typeof(bool).FullName, // to bool
                        new LanguageSpecificString(
                            () => "{0} ? {0}->Value : false",
                            () => "{0} ? {0}.Value() : false",
                            () => "{0} ?? false",
                            () => "If({0}, False)")
                    },
                    {
                        KnownTypes.Visibility, // to visibility
                        new LanguageSpecificString(
                            () => $"{{0}} && {{0}}->Value ? ::{KnownTypes.VisibilityColonized}::Visible : ::{KnownTypes.VisibilityColonized}::Collapsed",
                            () => $"{{0}} && {{0}}.Value() ? ::winrt::{KnownTypes.VisibilityColonized}::Visible : ::winrt::{KnownTypes.VisibilityColonized}::Collapsed",
                            () => $"({{0}} ?? false) ? global::{KnownTypes.Visibility}.Visible : global::{KnownTypes.Visibility}.Collapsed",
                            () => $"If(If({{0}}, False), Global.{KnownTypes.Visibility}.Visible, Global.{KnownTypes.Visibility}.Collapsed)")
                    },
                }
            },
            {
                KnownTypes.Visibility, // from visibility
                new Dictionary<string, LanguageSpecificString>()
                {
                    {
                        typeof(bool).FullName, // to bool
                        new LanguageSpecificString(
                            () => $"{{0}} == ::{KnownTypes.VisibilityColonized}::Visible",
                            () => $"{{0}} == ::winrt::{KnownTypes.VisibilityColonized}::Visible",
                            () => $"{{0}} == global::{KnownTypes.Visibility}.Visible",
                            () => $"{{0}} = Global.{KnownTypes.Visibility}.Visible")
                    },
                    {
                        "System.Nullable`1<Boolean>", // to bool? - XamlSchemaCodeInfo.GetFullGenericNestedName(typeof(bool?), true)
                        new LanguageSpecificString(
                            () => $"{{0}} == ::{KnownTypes.VisibilityColonized}::Visible",
                            () => $"{{0}} == ::winrt::{KnownTypes.VisibilityColonized}::Visible",
                            () => $"{{0}} == global::{KnownTypes.Visibility}.Visible",
                            () => $"{{0}} = Global.{KnownTypes.Visibility}.Visible")
                    },
                    {
                        "Windows.Foundation.IReference`1<Boolean>", // to cx nullable bool
                        new LanguageSpecificString(
                            () => $"{{0}} == ::{KnownTypes.VisibilityColonized}::Visible",
                            () => $"{{0}} == ::winrt::{KnownTypes.VisibilityColonized}::Visible",
                            () => $"{{0}} == global::{KnownTypes.Visibility}.Visible",
                            () => $"{{0}} = Global.{KnownTypes.Visibility}.Visible")
                    },
                }
            },
        };
        #endregion

        internal static bool ImplementsXamlINotifyPropertyChanged(this XamlType type)
        {
            if (type.ImplementsINotifyPropertyChanged())
            {
                DirectUIXamlType duiType = GetDirectUIXamlType(type);
                return duiType.HasInterface(KnownTypes.XamlINotifyPropertyChanged);
            }
            return false;
        }

        internal static bool ImplementsXamlINotifyDataErrorInfo(this XamlType type)
        {
            if (type.ImplementsINotifyDataErrorInfo())
            {
                DirectUIXamlType duiType = GetDirectUIXamlType(type);
                return duiType.HasInterface(KnownTypes.XamlINotifyDataErrorInfo);
            }
            return false;
        }

        internal static bool ImplementsXamlINotifyCollectionChanged(this XamlType type)
        {
            if (type.ImplementsINotifyCollectionChanged())
            {
                DirectUIXamlType duiType = GetDirectUIXamlType(type);
                return duiType.HasInterface(KnownTypes.XamlINotifyCollectionChanged);
            }
            return false;
        }

        internal static bool ImplementsIInputValidationControl(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType.HasInterface(KnownTypes.IInputValidationControl);
        }

        internal static bool IsDerivedFromValidationCommand(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDerivedFromValidationCommand : false;
        }

        internal static bool IsDeprecated(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDeprecated : false;
        }

        internal static bool IsDerivedFromFrameworkTemplate(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDerivedFromFrameworkTemplate : false;
        }

        internal static bool IsDerivedFromDataTemplate(this XamlType type)
        {
            DirectUISchemaContext schema = type.SchemaContext as DirectUISchemaContext;
            return schema.DirectUISystem.DataTemplate.IsAssignableFrom(type.UnderlyingType);
        }

        internal static bool IsDerivedFromControlTemplate(this XamlType type)
        {
            DirectUISchemaContext schema = type.SchemaContext as DirectUISchemaContext;
            return schema.DirectUISystem.ControlTemplate.IsAssignableFrom(type.UnderlyingType);
        }

        internal static bool IsDerivedFromResourceDictionary(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDerivedFromResourceDictionary : false;
        }

        internal static bool IsDerivedFromUIElement(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDerivedFromUIElement : false;
        }

        internal static bool IsDerivedFromWindow(this XamlType type)
        {
            DirectUISchemaContext schema = type.SchemaContext as DirectUISchemaContext;
            return schema.DirectUISystem.Window.IsAssignableFrom(type.UnderlyingType);
        }

        internal static bool IsDerivedFromFlyoutBase(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDerivedFromFlyoutBase : false;
        }

        internal static bool IsDerivedFromMarkupExtension(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDerivedFromMarkupExtension : false;
        }

        internal static bool IsDerivedFromTextBox(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDerivedFromTextBox : false;
        }

        internal static bool IsDelegate(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.IsDelegate : false;
        }

        internal static bool IsObject(this XamlType type)
        {
            return type.UnderlyingType.FullName == typeof(Object).ToString();
        }

        internal static bool IsVoid(this XamlType type)
        {
            return type.UnderlyingType.FullName == typeof(void).ToString();
        }

        internal static bool IsBoolOrNullableBool(this XamlType xamlType)
        {
            var type = xamlType.UnderlyingType;
            var typeName = type.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(type, true) // ignore assembly in generic type names
                : type.FullName;
            return typeName == typeof(bool).FullName
                || typeName == XamlSchemaCodeInfo.GetFullGenericNestedName(typeof(bool?), true) // nullable bool
                || typeName == "Windows.Foundation.IReference`1<Boolean>"; // cx nullable bool
        }

        private static DirectUIXamlType GetDirectUIXamlType(XamlType type)
        {
            Debug.Assert(type is DirectUIXamlType, String.Format(
                "'{0}' is not a DirectUIXamlType, are you calling this in Pass1 for a local type", type.ToString()));
            return type as DirectUIXamlType;
        }
        private static bool CacheImplicitCast(Type source, Type target, bool castable)
        {
            var sourceTypeName = source.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(source, true) // ignore assembly in generic type names
                : source.FullName;
            var targetTypeName = target.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(target, true) // ignore assembly in generic type names
                : target.FullName;

            if (!implicitCastCache.ContainsKey(sourceTypeName))
            {
                implicitCastCache.Add(sourceTypeName, new Dictionary<string, bool>());
            }

            if (!implicitCastCache[sourceTypeName].ContainsKey(targetTypeName))
            {
                implicitCastCache[sourceTypeName].Add(targetTypeName, castable);
            }
            else
            {
                Debug.Assert(false, "Are you calling change the cache value? Not supported.");
            }

            return castable;
        }

        private static bool IsImplicitlyCastableTo(Type source, Type target)
        {
            var sourceTypeName = source.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(source, true) // ignore assembly in generic type names
                : source.FullName;
            var targetTypeName = target.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(target, true) // ignore assembly in generic type names
                : target.FullName;

            if (implicitCastCache.ContainsKey(sourceTypeName))
            {
                if (implicitCastCache[sourceTypeName].ContainsKey(targetTypeName))
                {
                    return implicitCastCache[sourceTypeName][targetTypeName];
                }
            }

            if (primitiveTypesCasting.ContainsKey(sourceTypeName))
            {
                if (primitiveTypesCasting[sourceTypeName].Contains(targetTypeName))
                {
                    return CacheImplicitCast(source, target, true);
                }
            }

            // first ask the source if it's aware of an implicit conversion
            MethodInfo[] methods = source.GetMethods(BindingFlags.Public | BindingFlags.Static);
            foreach (MethodInfo method in methods)
            {
                if (method.ReturnType == target && method.Name.Equals(KnownStrings.op_Implicit))
                {
                    return CacheImplicitCast(source, target, true);
                }
            }

            // then ask the target if it's aware of an implicit conversion
            methods = target.GetMethods(BindingFlags.Public | BindingFlags.Static);
            foreach (MethodInfo method in methods)
            {
                if (method.ReturnType == target && method.Name.Equals(KnownStrings.op_Implicit))
                {
                    // make sure the input to this method is the source type
                    var parameters = method.GetParameters();
                    if (parameters[0].ParameterType == source)
                    {
                        return CacheImplicitCast(source, target, true);
                    }
                }
            }

            return CacheImplicitCast(source, target, false);
        }
        
        // All these methods are checking for something like "target = source"
        internal static bool CanAssignDirectlyTo(this XamlType source, XamlType target)
        {
            return CanAssignDirectlyTo(source, target.UnderlyingType);
        }

        internal static bool CanAssignDirectlyTo(this XamlType source, Type targetType)
        {
            return CanAssignDirectlyTo(source.UnderlyingType, targetType);
        }

        internal static bool CanAssignDirectlyTo(this Type sourceType, Type targetType)
        {
            return CanAssignDirectlyWithNoImplicitCast(sourceType, targetType) || IsImplicitlyCastableTo(sourceType, targetType);
        }

        internal static bool CanAssignDirectlyWithNoImplicitCast(this XamlType source, Type targetType)
        {
            return CanAssignDirectlyWithNoImplicitCast(source.UnderlyingType, targetType);
        }

        internal static bool CanAssignDirectlyWithNoImplicitCast(this Type sourceType, Type targetType)
        {
            return sourceType.FullName == targetType.FullName ||
                targetType.IsAssignableFrom(sourceType);
        }

        // Returns true and populates the conversion cache if there's a valid boxing conversion from source to target.
        // We can't short-circuit in the beginning by checking the inline conversion cache since the conversion
        // in it may be a non-boxing conversion.
        // Examples of conversions this handles:
        // int -> Nullable<int>
        // int -> Nullable<double>
        // Examples of conversions this method does NOT handle:
        // Nullable<int> -> int
        // Nullable<int> -> Nullable<double>
        internal static bool CanBoxTo(this XamlType source, XamlType target)
        {
            var sourceType = source.UnderlyingType;
            var sourceTypeName = sourceType.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(sourceType, true) // ignore assembly in generic type names
                : sourceType.FullName;
            var targetType = target.UnderlyingType;
            var targetTypeName = targetType.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(targetType, true) // ignore assembly in generic type names
                : targetType.FullName;

            LanguageSpecificString expressions = null;
            Dictionary<string, LanguageSpecificString> fromTypeLookup = null;

            // Are we trying to cast to a nullable/reference type from its wrapped value type?  If so we can box in-line
            if (target.IsBoxedType())
            {
                Type wrappedType = targetType.GetGenericArguments()[0];
                XamlType boxedXamlType = target.SchemaContext.GetXamlType(wrappedType);

                // Represents the final value we want to box.
                LanguageSpecificString boxedTypeExpressions = null;

                // If our wrapped type needs an inline conversion, get the convesion format string
                // E.g. for bool -> Nullable<Visibility>, we need to inline convert the Bool to Visibility first.
                if (!sourceType.CanAssignDirectlyTo(wrappedType) && CanInlineConvert(source, boxedXamlType))
                {
                    boxedTypeExpressions = GetInlineConversionFormats(source, boxedXamlType);
                }

                // If we need to manually box, do so
                // E.g. in managed code like C#/VB, we can assign directly from a T to a Nullable<T>,
                // so we don't need to explicitly box.
                // But in CX and CppWinRT, we need to explicitly box the T.
                if (!boxedXamlType.CanAssignDirectlyTo(target))
                {
                    string cppName = boxedXamlType.CppCXName();
                    string cSharpName = boxedXamlType.CSharpName();
                    string vbName = boxedXamlType.VBName();

                    string cppBoxedValue, cSharpBoxedValue, vbBoxedValue, cppWinRTBoxedValue;

                    // If the boxed type also requires an inline conversion, we want to make that inline conversion format string as the value that's boxed.
                    // Otherwise, stick in a {0} so the expression we're eventually given is boxed directly.
                    // The if block below handles the case where inline conversion isn't required, e.g. bool -> Platform::Box<bool>
                    if (boxedTypeExpressions == null)
                    {
                        // For CppWinRT, because box_value infers the IReference type to create based on the argument parameter,
                        // we need to cast it even if the type is directly assignable.  We only don't cast if the types are exactly equal
                        string cppWinRTCast;
                        if (sourceType.FullName.Equals(wrappedType.FullName))
                        {
                            cppWinRTCast = "{0}";
                        }
                        else
                        {
                            cppWinRTCast = source.CppWinRTCast(boxedXamlType, "{0}");
                        }

                        boxedTypeExpressions = new LanguageSpecificString(
                            () => "{0}",
                            () => cppWinRTCast,
                            () => "{0}",
                            () => "{0}");
                    }

                    cppBoxedValue = string.Format("ref new ::Platform::Box<{0}>({1})", cppName, boxedTypeExpressions.CppCXName());
                    cSharpBoxedValue = string.Format("new global::System.Nullable<{0}>({1})", cSharpName, boxedTypeExpressions.CSharpName());
                    vbBoxedValue = string.Format("New Global.System.Nullable (Of {0})({1})", vbName, boxedTypeExpressions.VBName());
                    cppWinRTBoxedValue = string.Format("winrt::box_value({0}).as<{1}>()", boxedTypeExpressions.CppWinRTName(), target.CppWinRTName());

                    expressions = new LanguageSpecificString(
                        () => cppBoxedValue,
                        () => cppWinRTBoxedValue,
                        () => cSharpBoxedValue,
                        () => vbBoxedValue
                    );
                }
            }

            if (InlineConversionsCache.ContainsKey(sourceTypeName))
            {
                fromTypeLookup = InlineConversionsCache[sourceTypeName];
            }

            if (fromTypeLookup == null)
            {
                InlineConversionsCache.Add(sourceTypeName, new Dictionary<string, LanguageSpecificString>());
                fromTypeLookup = InlineConversionsCache[sourceTypeName];
            }

            // Double-check our dictionary doesn't already have this mapping
            // If expressions is null, we don't want to insert null into the cache - if there really is no
            // possible inline conversion, CanInlineConvert will insert null for us
            if (!fromTypeLookup.ContainsKey(targetTypeName) && expressions != null)
            {
                fromTypeLookup.Add(targetTypeName, expressions);
            }

            return expressions != null;
        }

        internal static bool CanInlineConvert(this XamlType source, XamlType target)
        {
            var sourceType = source.UnderlyingType;
            var sourceTypeName = sourceType.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(sourceType, true) // ignore assembly in generic type names
                : sourceType.FullName;
            var targetType = target.UnderlyingType;
            var targetTypeName = targetType.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(targetType, true) // ignore assembly in generic type names
                : targetType.FullName;

            Dictionary<string, LanguageSpecificString> fromTypeLookup = null;

            // if we've reasoned about this type combination before...
            if (InlineConversionsCache.ContainsKey(sourceTypeName))
            {
                fromTypeLookup = InlineConversionsCache[sourceTypeName];
                if (fromTypeLookup.ContainsKey(targetTypeName))
                {
                    return fromTypeLookup[targetTypeName] != null;
                }
            }

            bool canExplicitlyCast = false;

            // if the target type is based on the source then we can attempt the cast
            if (targetType.IsSubclassOf(sourceType) // this also handles all object-to-class casts
                || sourceType.IsAssignableFrom(targetType)) // this handles interface to concrete casts
            {
                canExplicitlyCast = true;
            }

            // reverse primitive implicit cast lookup
            if (!canExplicitlyCast && primitiveTypesCasting.ContainsKey(targetTypeName))
            {
                var toTypeLookup = primitiveTypesCasting[targetTypeName];
                if (toTypeLookup.Contains(sourceTypeName))
                {
                    canExplicitlyCast = true;
                }
            }

            if (!canExplicitlyCast)
            {
                // does the source have an explicit cast to the target?
                MethodInfo[] methods = sourceType.GetMethods(BindingFlags.Public | BindingFlags.Static);
                foreach (MethodInfo method in methods)
                {
                    if (method.ReturnType == targetType && method.Name.Equals(KnownStrings.op_Explicit))
                    {
                        canExplicitlyCast = true;
                        break;
                    }
                }
            }

            if (!canExplicitlyCast)
            {
                // does the target have an explicit cast from the source?
                MethodInfo[] methods = targetType.GetMethods(BindingFlags.Public | BindingFlags.Static);
                foreach (MethodInfo method in methods)
                {
                    if (method.ReturnType == targetType && method.Name.Equals(KnownStrings.op_Explicit))
                    {
                        // make sure the input to this method is the source type
                        var parameters = method.GetParameters();
                        if (parameters[0].ParameterType == sourceType)
                        {
                            canExplicitlyCast = true;
                            break;
                        }
                    }
                }
            }

            LanguageSpecificString expressions = null;
            if (canExplicitlyCast)
            {
                string targetCppName = target.CppCXName();
                string targetCppWinRTName = target.CppWinRTName();
                string targetCSharpName = target.CSharpName();
                string targeVbName = target.VBName();
                string sourceVbCastName = source.GetVBCastName(target.UnderlyingType);
                var cppWinRTCast = source.CppWinRTCast(target, "{0}");
                expressions = new LanguageSpecificString(
                    () => string.Format("safe_cast<{0}>({{0}})", targetCppName),
                    () => cppWinRTCast,
                    () => string.Format("({0}){{0}}", targetCSharpName),
                    () => string.Format("{1}({{0}}, {0})", targeVbName, sourceVbCastName));
            }

            // If our reference/nullable type casting logic succeeds, or the cache already contains the source type name, we are guaranteed
            // the cache 
            if (CanBoxTo(source, target) || InlineConversionsCache.ContainsKey(sourceTypeName))
            {
                fromTypeLookup = InlineConversionsCache[sourceTypeName];
            }

            if (fromTypeLookup == null)
            {
                InlineConversionsCache.Add(sourceTypeName, new Dictionary<string, LanguageSpecificString>());
                fromTypeLookup = InlineConversionsCache[sourceTypeName];
            }

            // Double-check our dictionary doesn't already have this mapping, in-case CanBoxTo
            // inserted a boxing conversion.  We do allow expressions to be null in the insert to signal
            // there is no in-line conversion.
            if (!fromTypeLookup.ContainsKey(targetTypeName))
            {
                fromTypeLookup.Add(targetTypeName, expressions);
            }

            // Just in case CanBoxTo found something to use and we have it in the cache now
            if (expressions == null && fromTypeLookup.ContainsKey(targetTypeName))
            {
                expressions = fromTypeLookup[targetTypeName];
            }

            return expressions != null;
        }

        public static LanguageSpecificString GetInlineConversionExpression(this XamlType source, XamlType target, LanguageSpecificString memberExpression)
        {
            var expressions = GetInlineConversionFormats(source, target);
            return new LanguageSpecificString(
                () => expressions != null ? string.Format(expressions.CppCXName(), memberExpression.CppCXName()) : $"safe_cast<{target.CppCXName()}>({memberExpression.CppCXName()})",
                () => expressions != null ? string.Format(expressions.CppWinRTName(), memberExpression.CppWinRTName()) : source.CppWinRTCast(target, memberExpression.CppWinRTName()),
                () => expressions != null ? string.Format(expressions.CSharpName(), memberExpression.CSharpName()) : $"({target.CSharpName()}){memberExpression.CSharpName()}",
                () => expressions != null ? string.Format(expressions.VBName(), memberExpression.VBName()) : $"{source.GetVBCastName(target.UnderlyingType)}({memberExpression.VBName()}, {target.VBName()})");
            ;
        }

        internal static LanguageSpecificString GetInlineConversionFormats(this XamlType source, XamlType target)
        {
            var sourceType = source.UnderlyingType;
            var sourceTypeName = sourceType.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(sourceType, true) // ignore assembly in generic type names
                : sourceType.FullName;
            var targetType = target.UnderlyingType;
            var targetTypeName = targetType.IsGenericType
                ? XamlSchemaCodeInfo.GetFullGenericNestedName(targetType, true) // ignore assembly in generic type names
                : targetType.FullName;

            if (InlineConversionsCache.ContainsKey(sourceTypeName))
            {
                var sourceLookup = InlineConversionsCache[sourceTypeName];
                if (sourceLookup.ContainsKey(targetTypeName))
                {
                    return sourceLookup[targetTypeName];
                }
            }
            return null;
        }

        internal static bool IsEnum(this XamlType source)
        {
            return source.UnderlyingType.IsEnum;
        }

        internal static IEnumerable<string> GetEnumNames(this XamlType source)
        {
            if (IsEnum(source))
            {
                foreach (string enumValue in Enum.GetNames(source.UnderlyingType))
                {
                    yield return enumValue;
                }
            }
        }


        public static bool IsNullExtension(this XamlType instance)
        {
            DirectUISchemaContext duiSchema = instance.SchemaContext as DirectUISchemaContext;
            if (duiSchema != null && instance.CanAssignTo(duiSchema.DirectUIXamlLanguage.NullExtension))
            {
                return true;
            }
            return false;
        }

        public static string GetVBCastName(this XamlType source, Type targetType)
        {
            return CanAssignDirectlyWithNoImplicitCast(source, targetType) ? KnownStrings.DirectCast : KnownStrings.CType;
        }


        public static bool IsContractVersionAttribute(this Type type)
        {
            return type.FullName.Equals(KnownTypes.ContractVersionAttribute);
        }

        internal static bool HasCreateFromStringMethod(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.CreateFromStringMethod.Exists : false;
        }

        internal static bool HasMember(this XamlType type, string name)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType != null ? duiType.HasMember(name) : false;
        }

        internal static CreateFromStringMethod GetCreateFromStringMethod(this XamlType type)
        {
            DirectUIXamlType duiType = GetDirectUIXamlType(type);
            return duiType?.CreateFromStringMethod;
        }

        internal static XamlCompileError EnsureCreateFromStringResolved(this DirectUISchemaContext schemaContext, string declaringTypeName, CreateFromStringMethod createMethod, XamlDomNode locationForErrors)
        {
            // If we resolved the CreateFromString method successfully, we don't need to do anything else.
            // If we ran into errors, we treat the CreateFromString method as unresolved and want to
            // raise errors wherever we attempt to use the type.
            if (!createMethod.Resolved)
            {
                string methodFullName = createMethod.UnresolvedName;

                // methodFullName must always have a . added by CreateFromStringMethod in DirectUIXamlType,
                // even if the declaration in code contains only a local name.
                if (!methodFullName.Contains("."))
                {
                    return new XamlValidationCreateFromStringError(
                        declaringTypeName, methodFullName,
                        XamlCompilerResources.CreateFromString_MethodOnTypeNotFound, locationForErrors);
                }

                string methodName = methodFullName.Substring(methodFullName.LastIndexOf('.') + 1);
                string methodTypeName = methodFullName.Substring(0, methodFullName.LastIndexOf('.'));

                // Validate method is not null
                if (string.IsNullOrEmpty(methodName))
                {
                    return new XamlValidationCreateFromStringError(
                        declaringTypeName, methodFullName,
                        XamlCompilerResources.CreateFromString_MethodOnTypeNotFound, locationForErrors);
                }

                // Validate type exists
                XamlType methodType = schemaContext.GetXamlType(methodTypeName);
                if (methodType == null)
                {
                    return new XamlValidationCreateFromStringError(
                       declaringTypeName, methodFullName,
                       XamlCompilerResources.CreateFromString_TypeNotFound, locationForErrors);
                }

                // Resolve the type/method
                MethodInfo[] methods = methodType.UnderlyingType.GetMethods(BindingFlags.Public | BindingFlags.Static);
                bool foundMatchingMethod = false;
                bool foundMatchingMethodWithMatchingParameters = false;
                foreach (MethodInfo method in methods.Where(m => m.Name.Equals(methodName, StringComparison.OrdinalIgnoreCase)))
                {
                    foundMatchingMethod = true;
                    var parameters = method.GetParameters();
                    if (parameters.Length == 1)
                    {
                        if (parameters[0].ParameterType.FullName.Equals(typeof(string).FullName, StringComparison.OrdinalIgnoreCase) && !parameters[0].IsOut)
                        {
                            foundMatchingMethodWithMatchingParameters = true;
                            createMethod.SetResolved(methodType, methodName, method);
                            break;
                        }
                    }
                }

                // Validate method exists
                if (!foundMatchingMethod)
                {
                    return new XamlValidationCreateFromStringError(
                        declaringTypeName, methodFullName, XamlCompilerResources.CreateFromString_MethodOnTypeNotFound, locationForErrors);
                }

                // Validate method found has the right parameters
                if (foundMatchingMethod && !foundMatchingMethodWithMatchingParameters)
                {
                    return new XamlValidationCreateFromStringError(
                        declaringTypeName, methodFullName, XamlCompilerResources.CreateFromString_InvalidMethodSignature, locationForErrors);
                }
            }

            return null;
        }

        public static string NameWithApiInformation(this XamlMember member)
        {
            var duiMember = member as DirectUIXamlMember;
            if (duiMember?.ApiInformation != null)
            {
                return string.Format("{0}?{1}", member.ToString(), duiMember.ApiInformation.UniqueName);
            }
            else
            {
                return member.ToString();
            }
        }

        public static LanguageSpecificString GetStringToThing(this XamlType type, String valueName, bool isLiteral = false)
        {
            return GetStringToThing(type, new LanguageSpecificString(() => valueName), isLiteral);
        }

        public static LanguageSpecificString GetStringToThing(this XamlType type, LanguageSpecificString valueName, bool isLiteral = false)
        {
            // For boxed types, we want to do our string-to-thing conversion with the underlying boxed type, then box the result
            if (type.IsBoxedType())
            {
                XamlType boxedXamlType = type.SchemaContext.GetXamlType(type.GetBoxedType());

                LanguageSpecificString stringToThingInnerType = GetStringToThing(boxedXamlType, valueName, isLiteral);

                if (boxedXamlType.CanAssignDirectlyTo(type))
                {
                    return stringToThingInnerType;
                }
                else if (CanInlineConvert(boxedXamlType, type))
                {
                    // Sanity check we can convert, and ensure our conversion format strings are initialized
                    LanguageSpecificString boxedStringToThing = GetInlineConversionExpression(boxedXamlType, type, stringToThingInnerType);
                    return boxedStringToThing;
                }
                else
                {
                    // We should never hit this - we couldn't convert the inner type of the boxed type to its boxed type.  Our conversion logic
                    // in CanInlineConvert should always succeed.
                    throw new NotImplementedException($"Couldn't convert boxed type {boxedXamlType.UnderlyingType.FullName} to {type.UnderlyingType.FullName}");
                }
            }


            var cppWinRTValue = isLiteral ? $"L{valueName.CppWinRTName()}" : valueName.CppWinRTName();
            if (!type.HasCreateFromStringMethod())
            {
                return new LanguageSpecificString(
                    () => $"({type.CppCXName()}) ::{KnownTypes.XamlBindingHelperColonized}::ConvertValue({type.CppCXName(false)}::typeid, {valueName.CppCXName()})",
                    () =>
                    {
                        return type.NeedsBoxUnbox() ?
                    $"::winrt::unbox_value<{type.CppWinRTName()}>(::winrt::{KnownTypes.XamlBindingHelperColonized}::ConvertValue(::winrt::xaml_typename<{type.CppWinRTName()}>(), ::winrt::box_value(::winrt::hstring({cppWinRTValue}))))" :
                    type.SchemaContext.GetXamlType(typeof(Object)).CppWinRTCast(type, $"::winrt::{KnownTypes.XamlBindingHelperColonized}::ConvertValue(::winrt::xaml_typename<{type.CppWinRTName()}>(), ::winrt::box_value(::winrt::hstring({cppWinRTValue})))");
                    },
                    () => $"({type.CSharpName()}) global::{KnownTypes.XamlBindingHelper}.ConvertValue(typeof({type.CSharpName()}), {valueName.CSharpName()})",
                    () => $"DirectCast(Global.{KnownTypes.XamlBindingHelper}.ConvertValue(GetType({type.VBName()}), {valueName.VBName()}), {type.VBName()})"
                    );
            }
            else
            {
                var createFromString = type.GetCreateFromStringMethod();
                if (createFromString.MethodInfo.ReturnType.CanAssignDirectlyTo(type.UnderlyingType))
                {
                    // If the return type of the CreateFromStringMethod is exactly the same 
                    // as the type we want to create, we don't need to cast.
                    return new LanguageSpecificString(
                        () => { return String.Format("{0}({1})", createFromString.ResolvedName.CppCXName(), valueName.CppCXName()); },
                        () => { return String.Format("{0}({1})", createFromString.ResolvedName.CppWinRTName(), valueName.CppWinRTName()); },
                        () => { return String.Format("{0}({1})", createFromString.ResolvedName.CSharpName(), valueName.CSharpName()); },
                        () => { return String.Format("{0}({1})", createFromString.ResolvedName.VBName(), valueName.VBName()); }
                        );
                }
                else
                {
                    return new LanguageSpecificString(
                        () => { return String.Format("({0}){1}({2})", type.CppCXName(), createFromString.ResolvedName.CppCXName(), valueName.CppCXName()); },
                        () => $"::winrt::unbox_value<{type.CppWinRTName()}>({createFromString.ResolvedName.CppWinRTName()}({cppWinRTValue}))",
                        () => { return String.Format("({0}){1}({2})", type.CSharpName(), createFromString.ResolvedName.CSharpName(), valueName.CSharpName()); },
                        () => { return String.Format("DirectCast({1}({2}), {0})", type.VBName(), createFromString.ResolvedName.VBName(), valueName.VBName()); }
                        );
                }
            }
        }

        public static LanguageSpecificString ToStringWithNullCheckExpression(this XamlType type, LanguageSpecificString expression)
        {
            if (type.UnderlyingType.IsValueType)
            {
                return new LanguageSpecificString(
                    () => $"{expression.CppCXName()}.ToString()",
                    () => $"::winrt::to_hstring({expression.CppWinRTName()})",
                    () => $"{expression.CSharpName()}.ToString()",
                    () => $"{expression.VBName()}.ToString()");
            }
            else
            {
                return new LanguageSpecificString(
                    () => $"{expression.CppCXName()} != nullptr ? {expression.CppCXName()}->ToString() : nullptr",
                    () => $"::winrt::to_hstring({expression.CppWinRTName()})",
                    () => $"{expression.CSharpName()} != null ? {expression.CSharpName()}.ToString() : null",
                    () => $"If({expression.VBName()} IsNot Nothing, {expression.VBName()}.ToString(), Nothing)");
            }
        }

        public static bool NeedsBoxUnbox(this XamlType instance)
        {
            return instance.UnderlyingType.IsValueType || instance.IsString();
        }

        public static string CppWinRTCast(this XamlType source, XamlType target, string expression)
        {
            if (source.UnderlyingType.IsPrimitive && target.UnderlyingType.IsPrimitive)
            {
                return $"static_cast<{target.CppWinRTName()}>({expression})";
            }
            else if (target.IsString() && !source.IsString())
            {
                return $"::winrt::to_hstring({expression})";
            }
            else if (!source.UnderlyingType.IsPrimitive && !target.UnderlyingType.IsPrimitive)
            {
                return $"{expression}.as<{target.CppWinRTName()}>()";
            }
            else
            {
                return $"unknown_cast<{target.CppWinRTName()}>({expression})";
            }
        }

        public static string CppWinRTLocalElseRef(this XamlType type)
        {
            if (DomHelper.IsLocalType(type))
            {
                return type.CppWinRTName().ToLocalCppWinRTTypeName();
            }
            else
            {
                return type.CppWinRTName();
            }
        }

        public static bool HasFullXamlMetadataProviderAttribute(this Type type)
        {
            var exists = Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.GetCustomAttributeData(
                type, false, KnownTypes.FullXamlMetadataProviderAttribute).Any();
            return exists;
        }

        public static bool IsBoxedType(this XamlType type)
        {
            return IsBoxedType(type.UnderlyingType);
        }

        public static bool IsBoxedType(this Type type)
        {
            var typeFullName = type.FullName;
            return typeFullName.StartsWith(KnownTypes.Nullable) || typeFullName.StartsWith(KnownTypes.IReference);
        }

        public static Type GetBoxedType(this XamlType type)
        {
            return GetBoxedType(type.UnderlyingType);
        }

        public static Type GetBoxedType(this Type type)
        {
            Debug.Assert(IsBoxedType(type));
            return type.GetGenericArguments()[0];
        }

        public static string TryGetInputPropertyName(this XamlType type)
        {
            string memberName = null;
            var attributeData = Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.GetCustomAttributeData(type.UnderlyingType, false, KnownTypes.InputPropertyAttribute).SingleOrDefault();
            if (attributeData != null)
            {
                memberName = attributeData.NamedArguments[0].TypedValue.Value.ToString();
            }

            return memberName;
        }
    }
}
