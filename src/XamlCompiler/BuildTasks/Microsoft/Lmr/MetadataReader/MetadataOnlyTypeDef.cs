// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal;
using System.Collections.Generic;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.InteropServices;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Represent a TypeDef handle. It can contain generic arguments: opened, closed or partialy closed.
    /// See http://msdn.microsoft.com/en-us/library/system.type.isgenerictype.aspx for a list of key invariants
    /// of generic-related properties on System.Type.
    /// </summary>
    internal class MetadataOnlyTypeDef : MetadataOnlyCommonType
    {
        // The scope and handle within that scope that describe this type.
        readonly private MetadataOnlyModule m_resolver;
        readonly private TypeDefinitionHandle m_typeDefHandle;

        // Generic type arguments: "normal" types, type variables, or mix of both.
        readonly private Type[] m_typeParameters;

        // Handle for our base class.
        readonly private EntityHandle m_baseTypeHandle;

        // Full name including namespaces.
        private string m_fullName;

        // TypeAttributes value
        readonly private TypeAttributes m_typeAttributes;

        // Cached value of base-type. Lazily inited.
        // Thread-safety: ok to initialize multiple times.
        private Type m_baseType;

        public MetadataOnlyTypeDef(MetadataOnlyModule scope, TypeDefinitionHandle typeDefHandle)
            : this (scope, typeDefHandle, null)
        {
        }

        /// <summary>
        /// Creates LMR representation for TypeDef handle.
        /// </summary>
        /// <param name="scope">Module in which TypeDef is defined.</param>
        /// <param name="typeDefHandle">TypeDefinitionHandle representing a type.</param>
        /// <param name="typeParameters">Generic type arguments, if this is generic type.</param>
        /// <remarks>
        /// If this type represents generic type instantiation, here is how it's represented in metadata.
        /// From the Ecma spec:
        ///  TypeSpecBlob ::=
        ///    GENERICINST (CLASS | VALUETYPE) TypeDefOrRefEncoded GenArgCount Type Type*
        /// TypeDefOrRefEncoded can be a TypeRef,Def or Spec token. (And spec can be anything, like an Array)
        /// </remarks>
        public MetadataOnlyTypeDef(MetadataOnlyModule scope, TypeDefinitionHandle typeDefHandle, Type[] typeParameters)
        {
            if (scope == null)
            {
                throw new ArgumentNullException("scope");
            }

            m_resolver = scope;
            m_typeDefHandle = typeDefHandle;
            m_typeParameters = null;

            // Get type attributes and base type handle.
            m_resolver.GetTypeAttributes(m_typeDefHandle, out m_baseTypeHandle, out m_typeAttributes);

            // In the executable compiler, LMR reports public types as being imported and non-public for a yet-unknown reason.  Fix that up here
            if ((m_typeAttributes & TypeAttributes.Import) != 0)
            {
                m_typeAttributes = (m_typeAttributes & ~TypeAttributes.Import) | TypeAttributes.Public;
            }

            // Initialize generic parameters if there are any.

            int genericParamCount = m_resolver.CountGenericParams((EntityHandle)m_typeDefHandle);
            bool typeParametersPresent = (typeParameters != null) && (typeParameters.Length > 0);

            if (genericParamCount > 0)
            {
                if (!typeParametersPresent)
                {
                    // Constructing fully open generic type.
                    m_typeParameters = new Type[genericParamCount];
                    int i = 0;
                    foreach (GenericParameterHandle gpHandle in m_resolver.GetGenericParameterHandles((EntityHandle)m_typeDefHandle))
                    {
                        m_typeParameters[i++] = m_resolver.Factory.CreateTypeVariable(m_resolver, gpHandle);
                    }
                }
                else
                {
                    // Constructing closed, paritialy closed, or open generic type.
                    // Number of parameters must match.
                    if (genericParamCount == typeParameters.Length)
                    {
                        m_typeParameters = typeParameters;
                    }
                    else
                    {
                        throw new ArgumentException(Resources.WrongNumberOfGenericArguments); 
                    }
                }
            }
            else
            {
                // Constructing non-generic type. Make m_typeParameters non-null so can
                // we avoid null checks elsewhere in the code.
                //
                // Even if we had some type params passed in, they might be coming from the larger context,
                // but they might not be needed for construction of this type. This should not be an error. 
                // See MOEventInfo.EventHandlerType property and UnitTest1.TestEvents2 for the
                // examples of this situation. 
                m_typeParameters = Type.EmptyTypes;
            }
        }

        private string LocalFullName
        {
            get
            {
                // Calculated on demand in case the type is filtered via bindingflags.
                if (string.IsNullOrEmpty(m_fullName))
                {
                    m_fullName = m_resolver.GetTypeName(m_typeDefHandle);
                }
                return m_fullName;
            }
        }

        internal override MetadataOnlyModule Resolver
        {
            get
            {
                return m_resolver;
            }
        }

        public override int MetadataToken 
        { 
            get 
            { 
                return MetadataTokens.GetToken(m_typeDefHandle); 
            } 
        }

        /// <summary>
        /// Composes FullName for this type. 
        /// </summary>
        /// <remarks>
        /// Generics should include fully qualified names of type parameters, enclosed in square brackets.
        /// E.g. typeof(List<int>).FullName returns
        ///   "System.Collections.Generic.List`1[[System.Int32, mscorlib, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089]]"
        /// This is different than ToString, which would return 
        ///   "System.Collections.Generic.List`1[System.Int32]"
        /// See unit tests in GenericsTests.cs for more information.
        /// </remarks>
        public override string FullName 
        {
            get 
            {
                if ((!this.IsGenericType || this.IsGenericTypeDefinition) &&
                    this.DeclaringType == null)
                {
                    // If this is not a generic and not nested, then simply return the local
                    // fullname without reallocating another string.
                    return LocalFullName;
                }
                else
                {
                    StringBuilder sb = StringBuilderPool.Get();
                    GetSimpleName(sb);

                    // Non-generic types or fully open generic types don't have list of
                    // generic parameters as part of their FullName.
                    if (!this.IsGenericType || this.IsGenericTypeDefinition)
                    {
                        string result = sb.ToString();
                        StringBuilderPool.Release(ref sb);
                        return result;
                    }

                    // If this is generic type that is fully or partialy closed,
                    // we need to inspect its generic arguments to construct FullName.

                    sb.Append("[");
                    Type[] genArgs = this.GetGenericArguments();
                    for (int i = 0; i < genArgs.Length; i++)
                    {
                        if (i > 0) // skip first argument
                        {
                            sb.Append(",");
                        }

                        sb.Append('[');
                        if (genArgs[i].FullName == null || genArgs[i].IsGenericTypeDefinition)
                        {
                            // If there are any type variables and this type is not generic type definitition 
                            // (which would mean that all of generic parameters are type's own type variables)
                            // that means that either:
                            // 1. We have mix of open and closed type parameters.
                            // 2. We have mix of type's own type variables and some other type's type variables.
                            // Additionaly, if any of generic arguments are generic type definitions themselves
                            // (i.e not fully closed), FullName should be null.
                            return null;
                        }
                        sb.Append(genArgs[i].AssemblyQualifiedName);
                        sb.Append(']');
                    }
                    sb.Append("]");

                    string resultingText = sb.ToString();
                    StringBuilderPool.Release(ref sb);
                    return resultingText;
                }
            }
        }

        /// <summary>
        /// Constructs simple portion of type's complete name i.e. it does not
        /// include generic argument list (if there is any).
        /// </summary>
        private void GetSimpleName(StringBuilder sb)
        {
            // this.DeclaringType refers to the outer class if this is nested class.
            // Note that this property has different meaning for type variable in which
            // case it refers to type that declared that type variable. 
            Type outerType = this.DeclaringType; 
            if (outerType != null)
            {
                sb.Append(outerType.FullName);
                sb.Append('+');
            }

            sb.Append(LocalFullName);
        }

        /// <summary>
        /// Gets namespace for this type.
        /// </summary>
        /// <remarks>
        /// E.g. fullName = System.Reflection.MethodInfo
        ///     namespace = System.Reflection
        /// </remarks>
        public override string Namespace
        {
            get 
            {
                if (this.DeclaringType != null) 
                {
                    return this.DeclaringType.Namespace;
                }
                return Utility.GetNamespaceHelper(LocalFullName);
            }
        }

        /// <summary>
        /// Constructs string representation for this type. 
        /// </summary>
        /// <remarks>
        /// ToString is different than FullName when there are generic parameters.
        /// </remarks>
        public override string ToString()
        {
            if (!this.IsGenericType)
            {
                return this.FullName;
            }
            else
            {
                StringBuilder sb = StringBuilderPool.Get();
                GetSimpleName(sb);
                sb.Append("[");

                Type[] genArgs = this.GetGenericArguments();
                for (int i = 0; i < genArgs.Length; i++)
                {
                    if (i != 0) // skip first argument
                    {
                        sb.Append(",");
                    }

                    sb.Append(genArgs[i].ToString());
                }
                sb.Append("]");
                
                string result = sb.ToString();
                StringBuilderPool.Release(ref sb);
                return result;
            }
        }

        /// <summary>
        /// Get the base type that this derives from.
        /// Null if the curernt type is System.Object. 
        /// </summary>
        public override Type BaseType
        {
            get
            {
                if (m_baseType == null)
                {
                    if (m_baseTypeHandle.IsNil)
                    {
                        return null; //System.Object has no base class
                    }
                    else
                    {
                        // If base-type is generic, it can't be a TypeDef since that can't specify the generic args.
                        // Eg, consider: Derived<T> : Base<T>.
                        // base type token is not the typedef for Base. Rather it's a type spec for signature token:
                        //    Generic Type("Base") (#gen args = 1) (Var #0)
                        //
                        // Return a proxy object that defers resolution. 
                        // This can let tools do some inspection of our base type without having to resolve.
                        // It also lets us encode TypeSpec information if appropriate.
                        m_baseType = this.m_resolver.ResolveTypeTokenInternal(m_baseTypeHandle, this.GenericContext);
                    }
                }

                return m_baseType;
            }
        }

        /// <summary>
        /// Determines if two types are the same.
        /// </summary>
        /// <remarks>
        /// EqualsImpl should not call FullName since that could create infinite recursion for generic
        /// types. FullName needs to call IsGenericTypeDefinition, which in turn needs to call EqualsImpl.
        /// </remarks>
        public override bool Equals(Type other)
        {
            if (other == null)
            {
                return false;
            }

            // Module match means the types are in the same module and loaded in 
            // the same universe. 
            if (!this.Module.Equals(other.Module))
            {
                return false;
            }

            bool isThisGeneric = this.IsGenericType;
            bool isOtherGeneric = other.IsGenericType;

            if (isThisGeneric != isOtherGeneric)
            {
                // one is generic and one is not
                return false;
            }

            // If neither of types is generic or both are generic and they are both in
            // the same module we can just compare their metadata tokens.
            if (this.MetadataToken != other.MetadataToken)
            {
                return false;
            }

            // If types are not generic we are done with checks.
            if (!isThisGeneric && !isOtherGeneric)
            {
                return true;
            }

            // If both types are generic, we need to compare all generic arguments.

            Type[] args1 = this.GetGenericArguments();
            Type[] args2 = other.GetGenericArguments();

            if (args1.Length != args2.Length)
            {
                return false;
            }

            for (int i = 0; i < args1.Length; i++)
            {
                if (!args1[i].Equals(args2[i]))
                {
                    return false;
                }
            }

            return true;
        }

        #region Modifiers

        /// <summary>
        /// Creates generic type from generic type definition.
        /// </summary>
        /// <remarks>
        /// See http://msdn.microsoft.com/en-us/library/system.type.makegenerictype.aspx for details on invariants here. 
        /// </remarks>
        public override Type MakeGenericType(Type[] argTypes)
        {
            if (argTypes == null)
            {
                throw new ArgumentNullException("argTypes");
            }

            // MSDN and Reflection's behavior require that IsGenerictypeDefinition must be true
            // in order to MakeGenericType.
            if (this.IsGenericTypeDefinition)
            {
                if (argTypes.Length == m_typeParameters.Length)
                {
                    return this.Resolver.Factory.CreateGenericType(this.Resolver, m_typeDefHandle, argTypes);
                }
                else
                {
                    throw new ArgumentException(Resources.WrongNumberOfGenericArguments);
                }
            }
            else
            {
                // You can't call MakeGenericType on a partially or fully instantiated type (like Pair<int, T>
                // or Pair<int, string>). Instead, call GetGenericTypeDefinition first to get a Pair`2, 
                // and then call MakeGenericType on that.
                throw new InvalidOperationException();
            }
        }

        public override bool IsEnum
        {
            get
            {
                // From standard I.8.5.2a, Enums shall derive from "System.Enum"
            // Determining a enum fundamentally requires resolution because we have to compare
            // the base type. We could get the base class name and compare it to "System.Enum",
            // but even then we'd need to do resolution to ensure that the base type was actually
            // from mscorlib. 
                Type enumType = m_resolver.AssemblyResolver.GetTypeXFromName("System.Enum");
                return enumType.Equals(this.BaseType);
            }
        }

        public override bool IsAssignableFrom(Type c)
        {
            return IsAssignableFromHelper(this, c);
        }

        // Helper function for determining assignability. 
        // This operates on non-LMR types.
        static internal bool IsAssignableFromHelper(Type current, Type target)
        {
            // MSDN's description of Type.IsAssignableFrom is woefully incomplete. 
            // Part of the challenge is that assignability can have language semantics regarding implicit conversions. 
            // For example, in C#, anything can be assigned to a System.Object, because C# language semantic is to 
            // automatically box. At the IL level, some things can't be directly assigned to object. 
            // So reflection's actual assignability rules appear to be random and adhoc.
            if (target == null)
                return false;

            if (current.Equals(target))
                return true;

            // If c is a subclass of this class, then c can be cast to this type.
            if (target.IsSubclassOf(current))
                return true;

            // true if this type a base type on one of the interface impl
            Type[] interfaces = target.GetInterfaces();
            for (int i = 0; i < interfaces.Length; i++)
            {
                if (interfaces[i].Equals(current))
                    return true;
                if (current.IsAssignableFrom(interfaces[i]))
                    return true;
            }

            // if c is a generic type parameter and the current Type represents one of the constraints of c
            if (target.IsGenericParameter)
            {
                Type[] constraints = target.GetGenericParameterConstraints();
                for (int i = 0; i < constraints.Length; i++)
                    if (IsAssignableFromHelper(current, constraints[i]))
                        return true;
            }

            //Pointer, interface, and array types can be assigned to System.Object.
            ITypeUniverse universeCurrent = Helpers.Universe(current);
            if (universeCurrent != null)            
            {
                if (current.Equals(universeCurrent.GetTypeXFromName("System.Object")))
                {
                    return target.IsPointer || target.IsInterface || target.IsArray;
                }
            }
            return false;

        }

        public override Type UnderlyingSystemType
        {
            get { return this; }
        }

        protected override bool IsValueTypeImpl()
        {
            // Don't do this eagerly in the ctor because it may invoke Assembly resolution. 
            if (m_fIsValueType == TriState.Maybe)
            {
                bool f = IsValueTypeHelper();
                if (f) 
                    m_fIsValueType = TriState.Yes;
                else 
                    m_fIsValueType = TriState.No;
            }
            if (m_fIsValueType == TriState.Yes)
                return true;
            if (m_fIsValueType == TriState.No)
                return false;
            
            Debug.Assert(false);
            return false;
        }

        // Is ValueType is taking 25% of the time in Fib(20)
        enum TriState
        {
            Yes,
            No,
            Maybe,
        }
        TriState m_fIsValueType = TriState.Maybe;




        bool IsValueTypeHelper()
        {
            MetadataOnlyModule resolver = this.Resolver;

            //System.Enum is a subtype of System.ValueType but itself is not a value type.
            Type enumType = resolver.AssemblyResolver.GetTypeXFromName("System.Enum");
            if (this.Equals(enumType))
            {
                return false;
            }
            Type valueType = resolver.AssemblyResolver.GetTypeXFromName("System.ValueType");
            
            //IsValueType is true if the type's base type is System.ValueType or the type is an enum type.
            return valueType.Equals(this.BaseType) || this.IsEnum;
        }


        protected override bool IsPrimitiveImpl()
        {
            // Need to also to check if the module of this type is mscorlib
            // Types defined in different modules can use the same full name as the primitive types.            
            if (!this.m_resolver.IsSystemModule())
            {
                return false;
            }


            string fullName = this.FullName; // cache the property
            foreach (string name in PrimitiveTypeNames)
            {
                if (name.Equals(fullName, StringComparison.Ordinal))
                {
                    return true;
                }
            }
            return false;
        }

        static private readonly string[] PrimitiveTypeNames
            = { "System.Boolean",
                "System.Char",
                "System.SByte",
                "System.Byte",
                "System.Int16",
                "System.UInt16",
                "System.Int32",
                "System.UInt32",
                "System.Int64",
                "System.UInt64",
                "System.Single",
                "System.Double",
                "System.IntPtr",
                "System.UIntPtr" };

        /// <summary>
        /// Determines if this is a generic type based on number of type arguments.
        /// </summary>
        public override bool IsGenericType
        {
            get
            {
                return m_typeParameters.Length > 0;
            }
        }

        /// <summary>
        /// Creates type array of generic arguments. Returns empty array if this
        /// is not generic type.
        /// </summary>
        public override Type[] GetGenericArguments()
        {
            return (Type[])m_typeParameters.Clone();
        }

        /// <summary>
        /// Gets generic type definition (aka template) if this is generic type.
        /// </summary>
        public override Type GetGenericTypeDefinition()
        {
            if (!this.IsGenericType)
            {
                throw new InvalidOperationException();
            }

            if (this.IsGenericTypeDefinition)
            {
                return this;
            }
            else
            {
                return this.Resolver.Factory.CreateSimpleType(this.Resolver, this.m_typeDefHandle);
            }
        }

        /// <summary>
        /// Determines if this is a generic type definition i.e. fully open generic type.
        /// </summary>
        /// <remarks>
        /// Important: this method should not be directly or indirectly called from EqualsImpl
        /// since it would cause infinite reqursion. E.g. that's why EqualsImpl does not call
        /// FullName or ToString(); both of these APIs call IsGenericTypeDefinition.
        /// </remarks>
        public override bool IsGenericTypeDefinition
        {
            get
            {
                if (!this.IsGenericType)
                {
                    return false;
                }

                // In order for type to be fully open two conditions need to be satisfied:
                //  1) All generic arguments must be type variables.
                //  2) All type variables must be defined on this type.

                Type[] genArgs = this.GetGenericArguments();
                foreach (Type t in genArgs)
                {
                    if (!t.IsGenericParameter)
                    {
                        return false;
                    }
                    else if (!t.DeclaringType.Equals(this))
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        public override Type GetElementType()
        {
            //To mimic the reflection.
            return null;
        }

        #endregion // Modifiers


        // Implementation of System.Type.GetFields(BindingFlags)
        public override FieldInfo[] GetFields(System.Reflection.BindingFlags flags)
        {
            return MetadataOnlyModule.GetFieldsOnType(this, flags);
        }

        //Need to go through all the fields here by calling the GetFields method
        //to handle the BindingFlags matching. 
        //We might be able to look up the field directly to improve the performance.
        public override FieldInfo GetField(string name, BindingFlags bindingAttr)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            StringComparison comparison = SignatureUtil.GetStringComparison(bindingAttr);

            FieldInfo[] fs = GetFields(bindingAttr);
            foreach (FieldInfo f in fs)
            {
                if (f.Name.Equals(name, comparison))
                {
                    return f;
                }
            }

            return null;
        }


        static internal PropertyInfo GetPropertyImplHelper(
            MetadataOnlyCommonType type,
            String name, BindingFlags bindingAttr, Binder binder, Type returnType,
            Type[] types, ParameterModifier[] modifiers)
        {
            //binder must be null for LMR.
            if (binder != null)
            {
                throw new NotSupportedException();
            }

            //ParameterModifier is not handled by LMR
            if (modifiers != null && modifiers.Length != 0)
            {
                throw new NotSupportedException();
            }

            StringComparison comparison = SignatureUtil.GetStringComparison(bindingAttr);

            // Need to return the property matching all the criteria and in the most
            //derived class.
            PropertyInfo[] ps = type.GetProperties(bindingAttr);
            foreach (PropertyInfo p in ps)
            {
                if (!p.Name.Equals(name, comparison))
                {
                    continue;
                }

                if (returnType != null)
                {
                    if (!p.PropertyType.Equals(returnType))
                    {
                        continue;
                    }
                }

                if (!PropertyParamTypesMatch(p, types))
                {
                    continue;
                }

                return p;
            }

            return null;
        }

        private Type[] _interfacesCache;

        public override Type[] GetInterfaces()
        {
            if (_interfacesCache == null)
            {
                _interfacesCache = GetAllInterfacesHelper(this);
            }
            return _interfacesCache;
        }

        //Get all the inherited interfaces for the type,
        // - include the interfaces that its base types implement.
        // - including interfaces that any of those interfaces derive from
        // - filtering out any duplicates.
        // This becomes a tree walk.
        static internal Type[] GetAllInterfacesHelper(MetadataOnlyCommonType type)
        {
            // Since this is a tree walk, and may likely need to resolve TypeReferences, this could be very slow. 
            // Some signature dumper runs show getting the type's interfaces taking 40%+ of the time.
            //             
            // Any node in the tree could be a non-LMR type, so we can't naively use non-LMR APIs for the walk.
            // Some perf experimentation showed that even special casing for LMR types to use a fast-path
            // didn't result in a noticable perf gain when dumping winforms.dll.
            
            // Suppose you have interfaces I1...I5, and classes C1..C3
            // I1 : I2,I3,I5
            // I2 : I4,I6
            // 
            // C1 : C2, I1, I5
            // C2 : C3, I2
            // C3 : I3,I4
            // 
            // The tyepof(C1).GetInterfaces() == {I1,I2,I3,I4,I5, I6 }
            // C3's is {I3,I4}. C2's is {I2,I3,I4,I6}.
            // 
            // A key perf win is to avoid redundant querying for an interface's base interfaces. 
            // If we already have the full closure of I1, we we don't want to requery typeof(I1).GetInterfaces().
            

            
            // Hash of interfaces to return. This algorithm maintains the invariant that if interface I is in the hash, 
            // then the closure of I is also in the hash. That way, if I is already in the hash, we can skip
            // calling I.GetInterfaces(). 
            HashSet<Type> interfaces = new HashSet<Type>();

            // Start with the base types, and then add in new interfaces from the derived types.
            if (type.BaseType != null)
            {
                // This calls through Type.GetInterfaces(), which is the public API and will do the flattening.
                var ifaces = type.BaseType.GetInterfaces();
                interfaces.UnionWith(ifaces);
            }

            // Now just add on interfaces that are declared in the current class.
            var newInterfaces = type.Resolver.GetInterfacesOnType(type);
            foreach (var i in newInterfaces)
            {
                if (interfaces.Contains(i))
                    continue;

                // The interace is not in the set. Add it along with its closure. 
                interfaces.Add(i);

                // Getting the children interface is the slow path and makes this a non-linear operation.
                var iChildren = i.GetInterfaces();
                interfaces.UnionWith(iChildren);
            }

            // Convert Hash back to an array to return.
            Type[] result = new Type[interfaces.Count];
            interfaces.CopyTo(result);
            return result;
        }

        public override Type GetInterface(string name, bool ignoreCase)
        {
            return MetadataOnlyModule.GetInterfaceHelper(GetInterfaces(), name, ignoreCase);
        }


        public IEnumerable<InterfaceImpl> GetInterfaceImpls()
        {
            foreach (var tk in this.Resolver.EnumerateInterfaceImplsOnType(this))
            {
                yield return new InterfaceImpl(this, tk);
            }
        }

        //Compare the mthod's parameter types with the type array.
        //The types need to be exactly the same.
        private static bool PropertyParamTypesMatch(PropertyInfo p, Type[] types)
        {
            if (types == null)
            {
                return true;
            }

            ParameterInfo[] ps = p.GetIndexParameters();

            if (ps.Length != types.Length)
            {
                return false;
            }

            int numParams = ps.Length;
            for (int i = 0; i < numParams; i++)
            {
                //Compare types by using FullName
                //Cannot use LMRType.Equals because the two types may have different resolvers.
                if (!ps[i].ParameterType.Equals(types[i]))
                    return false;
            }

            return true;
        }

        public override EventInfo[] GetEvents(System.Reflection.BindingFlags flags)
        {
            return MetadataOnlyModule.GetEventsOnType(this, flags);
        }

        public override EventInfo GetEvent(string name, System.Reflection.BindingFlags flags)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            StringComparison comparison = SignatureUtil.GetStringComparison(flags);

            EventInfo[] es = GetEvents(flags);
            foreach (EventInfo e in es)
            {
                if (e.Name.Equals(name, comparison))
                {
                    return e;
                }
            }

            return null;
        }

        //Need to go through all the nested types here by calling the GetNestedTypes method
        //to handle the BindingFlags matching. 
        //We might be able to look up the field directly to improve the performance.
        public override Type GetNestedType(string name, BindingFlags bindingAttr)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            StringComparison comparison = SignatureUtil.GetStringComparison(bindingAttr);

            Type[] nestedTypes = GetNestedTypes(bindingAttr);
            foreach (Type type in nestedTypes)
            {
                if (type.Name.Equals(name, comparison))
                {
                    return type;
                }
            }

            return null;
        }

        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            // This algorithm is extremely slow and takes 90% of the time when running a signature dumper on mscorlib.
            List<Type> l = new List<Type>(m_resolver.GetNestedTypesOnType(this, bindingAttr));
            return l.ToArray();
        }


        public override MemberInfo[] GetMember(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            MemberInfo[] members = GetMembers(bindingAttr);
            List<MemberInfo> l = new List<MemberInfo>();

            StringComparison comparison = SignatureUtil.GetStringComparison(bindingAttr);

            foreach (MemberInfo m in members)
            {
                if (!name.Equals(m.Name, comparison))
                {
                    continue;
                }

                if (type != m.MemberType && type != MemberTypes.All)
                {
                    continue;
                }

                l.Add(m);
            }

            return l.ToArray();
        }



        static internal MemberInfo[] GetMembersHelper(Type type, BindingFlags bindingAttr)
        {
            //return Methods, Constructors, Fields, Properties, Events, and Nested Types.
            List<MemberInfo> l = new List<MemberInfo>(type.GetMethods(bindingAttr));
            l.AddRange(type.GetConstructors(bindingAttr));
            l.AddRange(type.GetFields(bindingAttr));
            l.AddRange(type.GetProperties(bindingAttr));
            l.AddRange(type.GetEvents(bindingAttr));
            l.AddRange(type.GetNestedTypes(bindingAttr));
            return l.ToArray();
        }

        public override Guid GUID
        {
            get 
            { 
                //The GUID is accessible only if the GUID is specified with [GuidAttribute].
                var customAttribues = this.GetCustomAttributesData();
                foreach (CustomAttributeData cad in customAttribues)
                {
                    if (cad.Constructor.DeclaringType.FullName.Equals("System.Runtime.InteropServices.GuidAttribute"))
                    {
                        Debug.Assert(cad.ConstructorArguments.Count == 1);
                        CustomAttributeTypedArgument arg = cad.ConstructorArguments[0];
                        Debug.Assert(arg.ArgumentType.FullName.Equals("System.String"));
                        string value = (string)(arg.Value);
                        return new Guid(value);
                    }
                }

                //Otherwise, return an empty GUID.
                return Guid.Empty;
            }
        }

        protected override bool HasElementTypeImpl()
        {
            return false;
        }

        public override object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            throw new NotSupportedException();
        }

        protected override bool IsCOMObjectImpl()
        {
            throw new NotImplementedException();
        }

        public override StructLayoutAttribute StructLayoutAttribute
        {
            get
            {
                if (this.IsInterface)
                    return null;

                // C# uses attribute notation to specify StructLayoutAttribute, but it's not a regular
                // custom attribute. The data is burned directly into the metadata via the ClassLayout table.
                
                // Get packing and size from Metadata using SRM.
                var typeDef = m_resolver.RawReader.GetTypeDefinition(m_typeDefHandle);
                var layout = typeDef.GetLayout();
                
                uint dwPackSize = (uint)layout.PackingSize;
                uint ulClassSize = (uint)layout.Size;

                // Match CLR's behavior, which is commented as:
                // Metadata parameter checking should not have allowed 0 for packing size.
                // The runtime later converts a packing size of 0 to 8 so do the same here
                // because it's more useful from a user perspective. 
                if (dwPackSize == 0)
                {
                    dwPackSize = 8;
                }


                // Get LayoutKind from attributes
                LayoutKind l;
                switch (this.m_typeAttributes & TypeAttributes.LayoutMask)
                {
                    case TypeAttributes.SequentialLayout:
                        l = LayoutKind.Sequential;
                        break;

                    case TypeAttributes.AutoLayout:
                        l = LayoutKind.Auto;
                        break;

                    case TypeAttributes.ExplicitLayout:
                        l = LayoutKind.Explicit;
                        break;

                    default:
                        throw new InvalidOperationException(Resources.IllegalLayoutMask);
                }

                // Character set
                CharSet charset = CharSet.None;
                switch(this.m_typeAttributes & TypeAttributes.CustomFormatClass)
                {
                    case TypeAttributes.AutoLayout:
                        charset = CharSet.Ansi;
                        break;
                    case TypeAttributes.UnicodeClass:
                        charset = CharSet.Unicode;
                        break;
                    case TypeAttributes.AutoClass:
                        charset = CharSet.Auto;
                        break;
                }


                StructLayoutAttribute s = new StructLayoutAttribute(l);
                s.Size = (int) ulClassSize;
                s.Pack = (int)dwPackSize;
                s.CharSet = charset;

                return s;
            }
        }

        #region MemberInfo Members

        public override MemberTypes MemberType
        {
            get 
            {
                if (IsNested)
                {
                    return MemberTypes.NestedType;
                }
                else
                {
                    return MemberTypes.TypeInfo;
                }
            }
        }

        public override Type DeclaringType
        {
            get 
            { 
                //Return the enclosing type if the type is nested.
                Type enclosingType = m_resolver.GetEnclosingType(m_typeDefHandle);
                if (enclosingType != null)
                {
                    return enclosingType;
                }
                return null;
            }
        }

        public override string Name
        {
            get 
            {
                return Utility.GetTypeNameFromFullNameHelper(LocalFullName, this.IsNested);
            }
        }

        public override Assembly Assembly
        {
            get 
            {
                return m_resolver.Assembly; 
            }
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


        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            return m_typeAttributes;
        }
       
        #endregion


        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return this.Resolver.GetCustomAttributeData((EntityHandle)m_typeDefHandle);
        }

        public override System.Reflection.GenericParameterAttributes GenericParameterAttributes
        {
            get { throw new InvalidOperationException(Resources.ValidOnGenericParameterTypeOnly); }
        }


        protected override TypeCode GetTypeCodeImpl()
        {
            // TypeDefs can represent most any typecode.
            TypeCode tc = this.m_resolver.GetTypeCode(this);
            return tc;
        }

        // From MOCommonType.
        internal override IEnumerable<PropertyInfo> GetDeclaredProperties()
        {
            return this.Resolver.GetPropertiesOnDeclaredTypeOnly(
                this.m_typeDefHandle, this.GenericContext);
        }
        internal override IEnumerable<MethodBase> GetDeclaredMethods()
        {
            return this.Resolver.GetMethodBasesOnDeclaredTypeOnly(
                this.m_typeDefHandle, this.GenericContext, MetadataOnlyModule.EMethodKind.Methods);
        }

        // From MOCommonType.
        internal override IEnumerable<MethodBase> GetDeclaredConstructors()
        {
            return this.Resolver.GetMethodBasesOnDeclaredTypeOnly(
                this.m_typeDefHandle, this.GenericContext, MetadataOnlyModule.EMethodKind.Constructor);
        }

        /// <summary>
        /// Represents an interface implemented directly by another type
        /// </summary>
        internal class InterfaceImpl
        {
            internal InterfaceImpl(MetadataOnlyTypeDef owningType, InterfaceImplementationHandle implHandle)
            {
                m_owningType = owningType;
                m_implHandle = implHandle;
            }

            public IList<CustomAttributeData> GetCustomAttributesData()
            {
                return m_owningType.Resolver.GetCustomAttributeData((EntityHandle)m_implHandle);
            }

            public Type GetInterfaceType()
            {
                return m_owningType.Resolver.GetInterfaceTypeFromInterfaceImpl(m_owningType, m_implHandle);
            }

            public Type OwningType
            {
                get
                {
                    return m_owningType;
                }
            }

            public int MetadataToken
            {
                get 
                {
                    return MetadataTokens.GetToken(m_implHandle);
                }
            }

            private MetadataOnlyTypeDef m_owningType;
            private InterfaceImplementationHandle m_implHandle;
        } // end class InterfaceImpl

    } // end class MetadataOnlyTypeDef


} // namespace
