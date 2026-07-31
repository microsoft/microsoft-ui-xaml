// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.Text;
using System.Globalization;
using System.Reflection.Adds;
using System.Diagnostics;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Base class helper for implementing a LMR Type that proxies to another type (such as a TypeRef or TypeSpec)
    /// </summary>
    /// <remarks>
    /// Explicitly have a DebuggerDisplay here to avoid 
    /// 1) inheriting from debug display of MetadataOnlyCommonType for calling Name and FullName,
    /// 2) the debugger implicitly calling ToString() here,
    /// which could throw exceptions when the type resolution fails.
    /// This derives from MetadataOnlyCommonType instead of System.Type directly because we can only have 1 base class,
    /// and derived classes need to derive from CommonType.
    /// 
    /// Important: TypeProxy must overload and forward all virtual methods from System.Type instead of providing its
    /// own implementation - even when this implementation is correct for all LMR types. This is because proxy could
    /// resolve to non-LMR representation of Type and they might have their own implementation to which we should
    /// forward to. 
    /// </remarks>
    [DebuggerDisplay("TypeProxy")]
    internal abstract class TypeProxy : MetadataOnlyCommonType, ITypeProxy
    {
        // Scope that the Ref token is valid in. Not to be confused with the scope that the token resolves to.
        // A MetadataOnlyModule (not just a reflection Module) is needed for MOCommonType. This provides the backpointer
        // to LMR services (such as the Factory) as well as the underlying metadata importer. 
        // The resolver also provides the backpointer 
        protected readonly MetadataOnlyModule m_resolver;

        // Cached value of the type that this resolves to. This is lazily set.
        // Since this implementation of the Type interface forwards every call to the resolved type, caching 
        // the resolved type avoids redundant lookups. 
        // Note that the caching prevents the resolution target from changing. Eg, caching prevents a policy
        // where a TypeRef first resolves to "unknown", and then when the user loads a new target assembly,
        // the TypeRef now resolves to the right type.
        Type m_cachedResolvedType;

        protected TypeProxy(MetadataOnlyModule resolver)
        {
            this.m_resolver = resolver;
            Debug.Assert(m_resolver != null);
        }

        ///  <summary>
        /// Implements MetadataOnlyCommonType.Resolver
        /// Unlike Type.DeclaringScope, this specifically returns a LMR module, not just a Reflection module.
        /// </summary>
        internal override MetadataOnlyModule Resolver
        {
            get { return m_resolver; }
        }

        public ITypeUniverse TypeUniverse
        {
            get { return m_resolver.AssemblyResolver; }
        }

        /// <summary>
        /// Implementation of ITypeProxy.GetResolvedType().
        /// Get the resolved type that we proxy to. 
        /// Every other method then forwards to this type.
        /// </summary>
        /// <remarks>
        /// Resolution is bad when it requires external information which we may not have or which may be 
        /// very difficult to get. This is the case for TypeRefs.
        /// It's not as bad for type aliasing, such as with TypeSpecs.
        /// 
        /// Note: this can still return a proxy i.e. not resolve completely.
        /// </remarks>
        public virtual Type GetResolvedType()
        {
            if (m_cachedResolvedType == null)
            {
                m_cachedResolvedType = GetResolvedTypeWorker();
            }
            return m_cachedResolvedType;
        }

        protected abstract Type GetResolvedTypeWorker();

        #region Get names
        /// <summary>
        /// We need to forward ToString() calls for correctness, but that means we have a non-trivial ToString()
        /// impl that may invoke assembly resolution callbacks. Therefore we don't want the debugger to call
        /// it implicitly via func-evals. So Use a DebuggerDisplay() attribute on this class to mitigate. 
        /// This isn't bullet proof since any other implicit eval could potentially call this ToString().
        /// </summary>
        public override string ToString()
        {
            // Forward ToString calls. This is essential for correctness
            // Eg, if a client calls Type.BaseType.ToString(), it needs to operate on the resolved
            // type and not the TypeReference.
            return GetResolvedType().ToString();
        }

        /// <summary>
        /// Helpers to get various pieces of the name.
        /// Derived class may be able to use their extra knowledge to avoid resolution here.
        /// Typenames can get complicated (especially with generics, especated chars), so it's hard
        /// to correctly compute the namespace,name from the FullName.
        /// </summary>
        public override string FullName
        {
            // FullName appears to need resolution for generics. 
            get { return GetResolvedType().FullName; }
        }

        public override string Namespace
        {
            get
            {
                return GetResolvedType().Namespace;
            }
        }
        public override string Name
        {
            get
            {
                return GetResolvedType().Name;
            }
        }
                
        public override string AssemblyQualifiedName
        {
            get { return GetResolvedType().AssemblyQualifiedName; }
        }

        #endregion // Get names


        #region Equality

        /// <summary>
        /// TypeRef's values for those must match that of the proxy.
        /// (eg this.GetHashCode() == this.GetResolvedType().GetHashCode()). 
        /// Since the resolved type does not need to be a MetadataOnlyCommonType, TypeRef can't just inherits
        /// MetadataOnlyCommonType's impls of GetHashCode().         
        /// </summary>
        public override int GetHashCode()
        {
            return GetResolvedType().GetHashCode();
        }

        public override bool Equals(object objOther)
        {
            Type t = objOther as Type;
            if (t == null)
            {
                return false;
            }

            // Chain to the Type based comparer.
            return Equals(t);
        }

        public override bool Equals(Type t)
        {
            if (t == null)
            {
                return false;
            }

            return GetResolvedType().Equals(t);
        }

        #endregion // Equality

        #region Algebra
        // Algebra operations. We should be able to make these work on a TypeRef without having to resolve.
        // These build up a type-tree.

        public override Type MakeByRefType()
        {
            // Build up the type tree without forcing a resolve.
            var t = this.Resolver.Factory.CreateByRefType(this);
            return t;        
        }

        public override Type MakePointerType()
        {
            return this.Resolver.Factory.CreatePointerType(this);
        }

        public override int GetArrayRank()
        {
            return GetResolvedType().GetArrayRank();
        }

        public override Type MakeGenericType(Type[] args)
        {            
            // Build up the type tree to avoid resolution.
            return new ProxyGenericType(this, args);
        }

        public override Type MakeArrayType()
        {
            return this.Resolver.Factory.CreateVectorType(this);
        }

        public override Type MakeArrayType(int rank)
        {
            return this.Resolver.Factory.CreateArrayType(this, rank);
        }

        #endregion // Algebra

        #region Type Members

        public override Module Module
        {
            get
            {
                return GetResolvedType().Module;
            }
        }

        public override Type BaseType
        {
            get
            {
                return GetResolvedType().BaseType;
            }
        }

        protected override bool IsArrayImpl()
        {
            return GetResolvedType().IsArray;
        }

        protected override bool IsByRefImpl()
        {
            return GetResolvedType().IsByRef;
        }

        protected override bool IsPointerImpl()
        {
            return GetResolvedType().IsPointer;
        }

        public override bool IsEnum
        {
            get
            {
                return GetResolvedType().IsEnum;
            }
        }

        protected override bool IsValueTypeImpl()
        {
            return GetResolvedType().IsValueType;
        }

        protected override bool IsPrimitiveImpl()
        {
            return GetResolvedType().IsPrimitive;
        }

        public override Type GetElementType()
        {
            return GetResolvedType().GetElementType();
        }

        // Token of the resolved type (a TypeDef), and not of the reference.
        public override int MetadataToken
        {
            get { return GetResolvedType().MetadataToken; }
        }

        public override Type[] GetGenericArguments()
        {
            return GetResolvedType().GetGenericArguments();
        }

        public override Type GetGenericTypeDefinition()
        {
            return GetResolvedType().GetGenericTypeDefinition();
        }

        public override bool ContainsGenericParameters
        {
            get
            {
                return GetResolvedType().ContainsGenericParameters;
            }
        }

        public override MethodInfo[] GetMethods(BindingFlags flags)
        {
            return GetResolvedType().GetMethods(flags);
        }

        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            return GetResolvedType().GetConstructors(bindingAttr);
        }

        public override bool IsAssignableFrom(Type c)
        {
            return GetResolvedType().IsAssignableFrom(c);
        }

        protected override bool IsContextfulImpl()
        {
            return GetResolvedType().IsContextful;
        }

        protected override bool IsMarshalByRefImpl()
        {
            return GetResolvedType().IsMarshalByRef;
        }

        public override bool IsSubclassOf(Type c)
        {
            return GetResolvedType().IsSubclassOf(c);
        }

        public override Type UnderlyingSystemType
        {
            get { return GetResolvedType().UnderlyingSystemType; }
        }

        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            return GetResolvedType().Attributes;
        }

        protected override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            // prevent GetMethod() on the resolved type from throwing a bogus ArgumentNullException when we delegate from GetMethodImpl() on the proxy type.  
            // Ideally, we would just call GetMethodImpl() directly on the resolved type, but we can’t do that because it’s protected. 
            if ((types == null) && (modifiers == null) && (binder == null) && (callConvention == CallingConventions.Any))
            {
                return GetResolvedType().GetMethod(name, bindingAttr);
            }
            else
            {
                return GetResolvedType().GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
            }
        }

        protected override PropertyInfo GetPropertyImpl(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            if ((types == null) && (modifiers == null))
            {
                // Reflection's implementation of the widest overload asserts that the parameters are non-null.
                // So we need to call the appropriate narrower overload. 
                if (returnType != null)
                {
                    return GetResolvedType().GetProperty(name, returnType);
                }
                else
                {
                    return GetResolvedType().GetProperty(name, bindingAttr);
                }
               
            }
            return GetResolvedType().GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
        }

        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return GetResolvedType().GetConstructor(bindingAttr, binder, callConvention, types, modifiers);
        }

        public override FieldInfo[] GetFields(BindingFlags flags)
        {
            return GetResolvedType().GetFields(flags);
        }

        public override PropertyInfo[] GetProperties(BindingFlags flags)
        {
            return GetResolvedType().GetProperties(flags);
        }

        public override EventInfo[] GetEvents(BindingFlags flags)
        {
            return GetResolvedType().GetEvents(flags);
        }

        public override EventInfo GetEvent(string name, System.Reflection.BindingFlags flags)
        {
            return GetResolvedType().GetEvent(name, flags);
        }

        public override FieldInfo GetField(string name, BindingFlags flags)
        {
            return GetResolvedType().GetField(name, flags);
        }

        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            return GetResolvedType().GetNestedTypes(bindingAttr);
        }

        public override Type GetNestedType(string name, BindingFlags bindingAttr)
        {
            return GetResolvedType().GetNestedType(name, bindingAttr);
        }

        public override MemberInfo[] GetMember(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            return GetResolvedType().GetMember(name, type, bindingAttr);
        }

        public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            return GetResolvedType().GetMembers(bindingAttr);
        }

        public override bool IsInstanceOfType(object o)
        {
            return GetResolvedType().IsInstanceOfType(o);
        }

        public override Type[] GetInterfaces()
        {
            return GetResolvedType().GetInterfaces();
        }

        public override Type GetInterface(string name, bool ignoreCase)
        {
            if (name == null) throw new ArgumentNullException("name");
            return GetResolvedType().GetInterface(name, ignoreCase);
        }

        public override bool IsGenericParameter
        {
            get
            {
                return GetResolvedType().IsGenericParameter;
            }
        }

        public override bool IsGenericType
        {
            get
            {
                return GetResolvedType().IsGenericType;
            }
        }

        public override bool IsGenericTypeDefinition
        {
            get
            {
                return GetResolvedType().IsGenericTypeDefinition;
            }
        }

        public override Guid GUID
        {
            get { return GetResolvedType().GUID; }
        }

        protected override bool HasElementTypeImpl()
        {
            return this.IsArray || this.IsByRef || this.IsPointer;
        }

        public override object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            return GetResolvedType().InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        }

        protected override bool IsCOMObjectImpl()
        {
            return GetResolvedType().IsCOMObject;
        }

        public override MemberInfo[] GetDefaultMembers()
        {
            return GetResolvedType().GetDefaultMembers();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            return GetResolvedType().GetCustomAttributesData();
        }

        public override StructLayoutAttribute StructLayoutAttribute
        {
            get
            {
                return GetResolvedType().StructLayoutAttribute;
            }
        }

        public override System.Reflection.GenericParameterAttributes GenericParameterAttributes
        {
            get { return GetResolvedType().GenericParameterAttributes; }
        }

        public override MethodBase DeclaringMethod
        {
            get
            {
                return GetResolvedType().DeclaringMethod;
            }
        }

        public override int GenericParameterPosition
        {
            get
            {
                return GetResolvedType().GenericParameterPosition;
            }
        }

        public override Type[] GetGenericParameterConstraints()
        {
            return GetResolvedType().GetGenericParameterConstraints();
        }

        public override System.Reflection.InterfaceMapping GetInterfaceMap(Type interfaceType)
        {
            return GetResolvedType().GetInterfaceMap(interfaceType);
        }

        public override Type[] FindInterfaces(TypeFilter filter, object filterCriteria)
        {
            return GetResolvedType().FindInterfaces(filter, filterCriteria);
        }

        public override MemberInfo[] GetMember(string name, BindingFlags bindingAttr)
        {
            return GetResolvedType().GetMember(name, bindingAttr);
        }

        public override bool IsSerializable
        {
            get
            {
                return GetResolvedType().IsSerializable;
            }
        }


        public override MemberInfo[] FindMembers(MemberTypes memberType, BindingFlags bindingAttr, MemberFilter filter, object filterCriteria)
        {
            return GetResolvedType().FindMembers(memberType, bindingAttr, filter, filterCriteria);
        }

        #endregion // Type Members

        #region MemberInfo Members

        public override MemberTypes MemberType
        {
            get { return GetResolvedType().MemberType; }
        }

        // Derived classes may be able to avoid doing resolution here. 
        public override Type DeclaringType
        {
            get
            {
                return GetResolvedType().DeclaringType;
            }
        }

        public override Assembly Assembly
        {
            get
            {
                return GetResolvedType().Assembly;
            }
        }

        public override object[] GetCustomAttributes(bool inherit)
        {
            return GetResolvedType().GetCustomAttributes(inherit);
        }

        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return GetResolvedType().GetCustomAttributes(attributeType, inherit);
        }

        public override bool IsDefined(Type attributeType, bool inherit)
        {
            return GetResolvedType().IsDefined(attributeType, inherit);
        }

        public override Type ReflectedType
        {
            get 
            {
                return GetResolvedType().ReflectedType;
            }
        }

        #endregion // MemberInfo Members

        protected override TypeCode GetTypeCodeImpl()
        {
            // Forward to resolved type.
            return Type.GetTypeCode(GetResolvedType());
        }

    } // end class TypeProxy
}
