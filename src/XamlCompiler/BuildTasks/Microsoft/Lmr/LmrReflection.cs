// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Globalization;
using System.IO;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Reflection over LMR objects (types, assemblies, modules and methods read from metadata) without going through the
    /// members of <see cref="Type"/>, <see cref="Assembly"/>, <see cref="Module"/> and <see cref="MethodBase"/> that are
    /// annotated for trimming, Native AOT and single-file.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The XAML compiler reflects over the types of the projects it compiles, which are LMR types, never runtime types: those
    /// annotations (and the warnings of the analyzers about them) don't apply to them. Each method here does the same thing as the
    /// member of the same name without the "Lmr" suffix (including the argument validation done by the non-virtual overloads of
    /// <see cref="Type"/>), but on an LMR object it calls the unannotated "*Core" implementation (see <see cref="LmrTypeBase"/>,
    /// <see cref="LmrAssemblyBase"/>, <see cref="MetadataOnlyModule"/>, <see cref="LmrMethodInfoBase"/> and <see cref="LmrConstructorInfoBase"/>).
    /// The names are different from the names of the reflection members on purpose: instance members always win over extension
    /// methods in overload resolution.
    /// </para>
    /// <para>
    /// For any other object (e.g. a runtime type), these forward to the reflection member on .NET Framework, so the
    /// behavior there doesn't change at all. In the Native AOT build (.NET 10) that would need the annotations: there, the
    /// members of <see cref="Type"/> only work on the closed set of runtime types in <c>KnownRuntimeTypes</c> (the
    /// built-in types of the XAML language), and everything else throws <see cref="NotSupportedException"/>.
    /// </para>
    /// </remarks>
    internal static class LmrReflection
    {
        // The same as Type.DefaultLookup
        private const BindingFlags DefaultLookup = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

        #region Type: constructors

        public static ConstructorInfo[] GetConstructorsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetConstructorsCore(BindingFlags.Public | BindingFlags.Instance);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetConstructors").GetConstructors();
#else
            return type.GetConstructors();
#endif
        }

        public static ConstructorInfo[] GetConstructorsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetConstructorsCore(bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetConstructors").GetConstructors(bindingAttr);
#else
            return type.GetConstructors(bindingAttr);
#endif
        }

        public static ConstructorInfo GetConstructorLmr(this Type type, Type[] types)
        {
            if (type is LmrTypeBase)
            {
                return GetConstructorLmr(type, BindingFlags.Public | BindingFlags.Instance, null, CallingConventions.Any, types, null);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetConstructor").GetConstructor(types);
#else
            return type.GetConstructor(types);
#endif
        }

        public static ConstructorInfo GetConstructorLmr(this Type type, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase)
            {
                return GetConstructorLmr(type, bindingAttr, binder, CallingConventions.Any, types, modifiers);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetConstructor").GetConstructor(bindingAttr, binder, types, modifiers);
#else
            return type.GetConstructor(bindingAttr, binder, types, modifiers);
#endif
        }

        public static ConstructorInfo GetConstructorLmr(this Type type, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateTypes(types);
                return lmrType.GetConstructorImplCore(bindingAttr, binder, callConvention, types, modifiers);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetConstructor").GetConstructor(bindingAttr, binder, callConvention, types, modifiers);
#else
            return type.GetConstructor(bindingAttr, binder, callConvention, types, modifiers);
#endif
        }

        #endregion

        #region Type: methods

        public static MethodInfo[] GetMethodsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMethodsCore(DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMethods").GetMethods();
#else
            return type.GetMethods();
#endif
        }

        public static MethodInfo[] GetMethodsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMethodsCore(bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMethods").GetMethods(bindingAttr);
#else
            return type.GetMethods(bindingAttr);
#endif
        }

        public static MethodInfo GetMethodLmr(this Type type, string name)
        {
            if (type is LmrTypeBase)
            {
                return GetMethodLmr(type, name, DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name);
#else
            return type.GetMethod(name);
#endif
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                return lmrType.GetMethodImplCore(name, bindingAttr, null, CallingConventions.Any, null, null);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, bindingAttr);
#else
            return type.GetMethod(name, bindingAttr);
#endif
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, Type[] types)
        {
            if (type is LmrTypeBase)
            {
                return GetMethodLmr(type, name, DefaultLookup, null, CallingConventions.Any, types, null);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, types);
#else
            return type.GetMethod(name, types);
#endif
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase)
            {
                return GetMethodLmr(type, name, bindingAttr, binder, CallingConventions.Any, types, modifiers);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, bindingAttr, binder, types, modifiers);
#else
            return type.GetMethod(name, bindingAttr, binder, types, modifiers);
#endif
        }

        public static MethodInfo GetMethodLmr(this Type type, string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                ValidateTypes(types);
                return lmrType.GetMethodImplCore(name, bindingAttr, binder, callConvention, types, modifiers);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMethod").GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
#else
            return type.GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
#endif
        }

        #endregion

        #region Type: properties

        public static PropertyInfo[] GetPropertiesLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetPropertiesCore(DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetProperties").GetProperties();
#else
            return type.GetProperties();
#endif
        }

        public static PropertyInfo[] GetPropertiesLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetPropertiesCore(bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetProperties").GetProperties(bindingAttr);
#else
            return type.GetProperties(bindingAttr);
#endif
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name)
        {
            if (type is LmrTypeBase)
            {
                return GetPropertyLmr(type, name, DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name);
#else
            return type.GetProperty(name);
#endif
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                return lmrType.GetPropertyImplCore(name, bindingAttr, null, null, null, null);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name, bindingAttr);
#else
            return type.GetProperty(name, bindingAttr);
#endif
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name, Type returnType)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                if (returnType == null)
                {
                    throw new ArgumentNullException(nameof(returnType));
                }
                return lmrType.GetPropertyImplCore(name, DefaultLookup, null, returnType, null, null);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name, returnType);
#else
            return type.GetProperty(name, returnType);
#endif
        }

        public static PropertyInfo GetPropertyLmr(this Type type, string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            if (type is LmrTypeBase lmrType)
            {
                ValidateName(name);
                if (types == null)
                {
                    throw new ArgumentNullException(nameof(types));
                }
                return lmrType.GetPropertyImplCore(name, bindingAttr, binder, returnType, types, modifiers);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetProperty").GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
#else
            return type.GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
#endif
        }

        #endregion

        #region Type: fields

        public static FieldInfo[] GetFieldsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldsCore(DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetFields").GetFields();
#else
            return type.GetFields();
#endif
        }

        public static FieldInfo[] GetFieldsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldsCore(bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetFields").GetFields(bindingAttr);
#else
            return type.GetFields(bindingAttr);
#endif
        }

        public static FieldInfo GetFieldLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldCore(name, DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetField").GetField(name);
#else
            return type.GetField(name);
#endif
        }

        public static FieldInfo GetFieldLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetFieldCore(name, bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetField").GetField(name, bindingAttr);
#else
            return type.GetField(name, bindingAttr);
#endif
        }

        #endregion

        #region Type: events

        public static EventInfo[] GetEventsLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventsCore(DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetEvents").GetEvents();
#else
            return type.GetEvents();
#endif
        }

        public static EventInfo[] GetEventsLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventsCore(bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetEvents").GetEvents(bindingAttr);
#else
            return type.GetEvents(bindingAttr);
#endif
        }

        public static EventInfo GetEventLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventCore(name, DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetEvent").GetEvent(name);
#else
            return type.GetEvent(name);
#endif
        }

        public static EventInfo GetEventLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetEventCore(name, bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetEvent").GetEvent(name, bindingAttr);
#else
            return type.GetEvent(name, bindingAttr);
#endif
        }

        #endregion

        #region Type: nested types

        public static Type[] GetNestedTypesLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypesCore(BindingFlags.Public);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetNestedTypes").GetNestedTypes();
#else
            return type.GetNestedTypes();
#endif
        }

        public static Type[] GetNestedTypesLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypesCore(bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetNestedTypes").GetNestedTypes(bindingAttr);
#else
            return type.GetNestedTypes(bindingAttr);
#endif
        }

        public static Type GetNestedTypeLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypeCore(name, BindingFlags.Public);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetNestedType").GetNestedType(name);
#else
            return type.GetNestedType(name);
#endif
        }

        public static Type GetNestedTypeLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetNestedTypeCore(name, bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetNestedType").GetNestedType(name, bindingAttr);
#else
            return type.GetNestedType(name, bindingAttr);
#endif
        }

        #endregion

        #region Type: interfaces

        public static Type[] GetInterfacesLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetInterfacesCore();
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetInterfaces").GetInterfaces();
#else
            return type.GetInterfaces();
#endif
        }

        public static Type GetInterfaceLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetInterfaceCore(name, false);
            }
#if NET10_0_OR_GREATER
            // Not supported on the known runtime types either: the compiler never needs it, and that keeps Type.GetInterface
            // unused in the native binary (see LmrTypeBase.GetInterface).
            throw NotLmr(type, "Type.GetInterface");
#else
            return type.GetInterface(name);
#endif
        }

        public static Type GetInterfaceLmr(this Type type, string name, bool ignoreCase)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetInterfaceCore(name, ignoreCase);
            }
#if NET10_0_OR_GREATER
            // See GetInterfaceLmr(Type, string)
            throw NotLmr(type, "Type.GetInterface");
#else
            return type.GetInterface(name, ignoreCase);
#endif
        }

        #endregion

        #region Type: members

        public static MemberInfo[] GetMembersLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMembersCore(DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMembers").GetMembers();
#else
            return type.GetMembers();
#endif
        }

        public static MemberInfo[] GetMembersLmr(this Type type, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMembersCore(bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMembers").GetMembers(bindingAttr);
#else
            return type.GetMembers(bindingAttr);
#endif
        }

        public static MemberInfo[] GetMemberLmr(this Type type, string name)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMemberCore(name, MemberTypes.All, DefaultLookup);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMember").GetMember(name);
#else
            return type.GetMember(name);
#endif
        }

        public static MemberInfo[] GetMemberLmr(this Type type, string name, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMemberCore(name, MemberTypes.All, bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMember").GetMember(name, bindingAttr);
#else
            return type.GetMember(name, bindingAttr);
#endif
        }

        public static MemberInfo[] GetMemberLmr(this Type type, string name, MemberTypes memberType, BindingFlags bindingAttr)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetMemberCore(name, memberType, bindingAttr);
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetMember").GetMember(name, memberType, bindingAttr);
#else
            return type.GetMember(name, memberType, bindingAttr);
#endif
        }

        public static MemberInfo[] GetDefaultMembersLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.GetDefaultMembersCore();
            }
#if NET10_0_OR_GREATER
            return KnownRuntimeTypes.Get(type, "Type.GetDefaultMembers").GetDefaultMembers();
#else
            return type.GetDefaultMembers();
#endif
        }

        public static object InvokeMemberLmr(this Type type, string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.InvokeMemberCore(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(type, "Type.InvokeMember");
#else
            return type.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
#endif
        }

        #endregion

        #region Type: type algebra

        public static Type MakeGenericTypeLmr(this Type type, params Type[] typeArguments)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeGenericTypeCore(typeArguments);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(type, "Type.MakeGenericType");
#else
            return type.MakeGenericType(typeArguments);
#endif
        }

        public static Type MakeArrayTypeLmr(this Type type)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeArrayTypeCore();
            }
#if NET10_0_OR_GREATER
            throw NotLmr(type, "Type.MakeArrayType");
#else
            return type.MakeArrayType();
#endif
        }

        public static Type MakeArrayTypeLmr(this Type type, int rank)
        {
            if (type is LmrTypeBase lmrType)
            {
                return lmrType.MakeArrayTypeCore(rank);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(type, "Type.MakeArrayType");
#else
            return type.MakeArrayType(rank);
#endif
        }

        #endregion

        #region Methods

        public static MethodInfo MakeGenericMethodLmr(this MethodInfo method, params Type[] typeArguments)
        {
            if (method is LmrMethodInfoBase lmrMethod)
            {
                return lmrMethod.MakeGenericMethodCore(typeArguments);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(method, "MethodInfo.MakeGenericMethod");
#else
            return method.MakeGenericMethod(typeArguments);
#endif
        }

        public static MethodBody GetMethodBodyLmr(this MethodBase method)
        {
            if (method is LmrMethodInfoBase lmrMethod)
            {
                return lmrMethod.GetMethodBodyCore();
            }
            if (method is LmrConstructorInfoBase lmrConstructor)
            {
                return lmrConstructor.GetMethodBodyCore();
            }
#if NET10_0_OR_GREATER
            throw NotLmr(method, "MethodBase.GetMethodBody");
#else
            return method.GetMethodBody();
#endif
        }

        #endregion

        #region Assembly

        public static Type[] GetTypesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypesCore();
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetTypes");
#else
            return assembly.GetTypes();
#endif
        }

        public static Type[] GetExportedTypesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetExportedTypesCore();
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetExportedTypes");
#else
            return assembly.GetExportedTypes();
#endif
        }

        public static Type GetTypeLmr(this Assembly assembly, string name)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetType");
#else
            return assembly.GetType(name);
#endif
        }

        public static Type GetTypeLmr(this Assembly assembly, string name, bool throwOnError)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name, throwOnError);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetType");
#else
            return assembly.GetType(name, throwOnError);
#endif
        }

        public static Type GetTypeLmr(this Assembly assembly, string name, bool throwOnError, bool ignoreCase)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetTypeCore(name, throwOnError, ignoreCase);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetType");
#else
            return assembly.GetType(name, throwOnError, ignoreCase);
#endif
        }

        public static AssemblyName[] GetReferencedAssembliesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetReferencedAssembliesCore();
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetReferencedAssemblies");
#else
            return assembly.GetReferencedAssemblies();
#endif
        }

        /// <summary>
        /// The same as <see cref="Assembly.Location"/>: the path of the metadata file of an LMR assembly.
        /// </summary>
        public static string GetLocationLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.LocationCore;
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.Location");
#else
            return assembly.Location;
#endif
        }

        public static string GetCodeBaseLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.CodeBaseCore;
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.CodeBase");
#else
            return assembly.CodeBase;
#endif
        }

        public static FileStream GetFileLmr(this Assembly assembly, string name)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFileCore(name);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetFile");
#else
            return assembly.GetFile(name);
#endif
        }

        public static FileStream[] GetFilesLmr(this Assembly assembly)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFilesCore();
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetFiles");
#else
            return assembly.GetFiles();
#endif
        }

        public static FileStream[] GetFilesLmr(this Assembly assembly, bool getResourceModules)
        {
            if (assembly is LmrAssemblyBase lmrAssembly)
            {
                return lmrAssembly.GetFilesCore(getResourceModules);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(assembly, "Assembly.GetFiles");
#else
            return assembly.GetFiles(getResourceModules);
#endif
        }

        #endregion

        #region Module

        public static Type[] GetTypesLmr(this Module module)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypesCore();
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.GetTypes");
#else
            return module.GetTypes();
#endif
        }

        public static Type GetTypeLmr(this Module module, string className)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, false, false);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.GetType");
#else
            return module.GetType(className);
#endif
        }

        public static Type GetTypeLmr(this Module module, string className, bool ignoreCase)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, false, ignoreCase);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.GetType");
#else
            return module.GetType(className, ignoreCase);
#endif
        }

        public static Type GetTypeLmr(this Module module, string className, bool throwOnError, bool ignoreCase)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.GetTypeCore(className, throwOnError, ignoreCase);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.GetType");
#else
            return module.GetType(className, throwOnError, ignoreCase);
#endif
        }

        public static Type ResolveTypeLmr(this Module module, int metadataToken)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveTypeCore(metadataToken, null, null);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.ResolveType");
#else
            return module.ResolveType(metadataToken);
#endif
        }

        public static Type ResolveTypeLmr(this Module module, int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveTypeCore(metadataToken, genericTypeArguments, genericMethodArguments);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.ResolveType");
#else
            return module.ResolveType(metadataToken, genericTypeArguments, genericMethodArguments);
#endif
        }

        public static MethodBase ResolveMethodLmr(this Module module, int metadataToken)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveMethodCore(metadataToken, null, null);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.ResolveMethod");
#else
            return module.ResolveMethod(metadataToken);
#endif
        }

        public static MethodBase ResolveMethodLmr(this Module module, int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.ResolveMethodCore(metadataToken, genericTypeArguments, genericMethodArguments);
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.ResolveMethod");
#else
            return module.ResolveMethod(metadataToken, genericTypeArguments, genericMethodArguments);
#endif
        }

        /// <summary>
        /// The same as <see cref="Module.Name"/>: the file name of the metadata file of an LMR module.
        /// </summary>
        public static string GetNameLmr(this Module module)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.NameCore;
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.Name");
#else
            return module.Name;
#endif
        }

        /// <summary>
        /// The same as <see cref="Module.FullyQualifiedName"/>: the path of the metadata file of an LMR module.
        /// </summary>
        public static string GetFullyQualifiedNameLmr(this Module module)
        {
            if (module is MetadataOnlyModule lmrModule)
            {
                return lmrModule.FullyQualifiedNameCore;
            }
#if NET10_0_OR_GREATER
            throw NotLmr(module, "Module.FullyQualifiedName");
#else
            return module.FullyQualifiedName;
#endif
        }

        #endregion

        #region Helpers

        // The same validation as the non-virtual overloads of Type.GetMethod/GetProperty/GetConstructor.
        private static void ValidateName(string name)
        {
            if (name == null)
            {
                throw new ArgumentNullException(nameof(name));
            }
        }

        private static void ValidateTypes(Type[] types)
        {
            if (types == null)
            {
                throw new ArgumentNullException(nameof(types));
            }
            for (int i = 0; i < types.Length; i++)
            {
                if (types[i] == null)
                {
                    throw new ArgumentNullException(nameof(types));
                }
            }
        }

#if NET10_0_OR_GREATER
        private static Exception NotLmr(object value, string member)
        {
            if (value == null)
            {
                // What calling the reflection member on null does
                return new NullReferenceException();
            }

            return new NotSupportedException(string.Format(CultureInfo.InvariantCulture,
                "{0} isn't supported on '{1}' ({2}) in the Native AOT build of the XAML compiler, which can only reflect over the types and assemblies it reads from metadata.",
                member, value, value.GetType().FullName));
        }
#endif

        #endregion
    }
}
