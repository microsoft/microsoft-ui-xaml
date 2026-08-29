// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.Text;
using System.Reflection.Adds;
using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Common base class for LMR derivations of Type.
    /// </summary>
    /// <remarks>
    /// LMR Type implementations need access comes to MetadataOnlyCommonType's Resolver Property.
    /// Types that are composed from LMR types (eg, a T&, T[], T<...>) may require the core MetadataOnlyCommonType
    /// (for T) as a argument.
    ///
    /// Internal interface exposing some more properties off Type.
    /// Public callers should use Type, so this interface here is internal.
    /// 
    /// Any new methods this class adds beyond Type should not be public. 
    /// </remarks>
    [System.Diagnostics.DebuggerDisplay(@"\{Name = {Name} FullName = {FullName}\}")]
    internal abstract class MetadataOnlyCommonType : Type
    {
        /// <summary>
        /// Get the resolver object that the type's TypeDef token is valid in.
        /// The resolver provides the tie back to LMR and LMR services, such as:
        /// - the raw metadata import,
        /// - the creation Factory.
        /// </summary>
        internal abstract MetadataOnlyModule Resolver
        {
            get;
        }

        internal virtual GenericContext GenericContext
        {
            get
            {
                return new GenericContext(this.GetGenericArguments(), null);
            }
        }
        #region Internal building block APIs for implementing reflection
        // These APIs are internal building blocks that can be used to provide
        // common implementations for other Reflection APIs.

        /// <summary>
        /// Internal helper for implementing Type.GetMethods().
        /// This returns *all* the methods declared on the current type including:
        /// - public, private
        /// - static, instance
        /// - fabricated (eg, on Array)
        /// This does not including any inherited methods. 
        /// 
        /// Type.GetMethods() is a complicated algorithm that uses this information. 
        /// </summary>
        /// <returns></returns>
        internal virtual IEnumerable<MethodBase> GetDeclaredMethods()
        {
            return new MethodInfo[0]; // default is empty
        }

        internal virtual IEnumerable<MethodBase> GetDeclaredConstructors()
        {
            return new MethodInfo[0]; // default is empty
        }

        internal virtual IEnumerable<PropertyInfo> GetDeclaredProperties()
        {
            return new PropertyInfo[0]; // default is empty
        }
        #endregion

        // Implementation of System.Type.GetProperties(BindingFlags)
        public override PropertyInfo[] GetProperties(System.Reflection.BindingFlags flags)
        {
            return MetadataOnlyModule.GetPropertiesOnType(this, flags);
        }

        protected override PropertyInfo GetPropertyImpl(
            String name, BindingFlags bindingAttr, Binder binder, Type returnType,
            Type[] types, ParameterModifier[] modifiers)
        {
            return MetadataOnlyTypeDef.GetPropertyImplHelper(this, name, bindingAttr, binder, returnType, types, modifiers);
        }

        public override MethodInfo[] GetMethods(System.Reflection.BindingFlags flags)
        {
            // This will call back on GetDeclaredMethods()
            return MetadataOnlyModule.GetMethodsOnType(this, flags);
        }

        protected override MethodInfo GetMethodImpl(string name, System.Reflection.BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, System.Reflection.ParameterModifier[] modifiers)
        {
            // This overload requests a specific Method. Common implementation is to build
            // this by searching over the list of all methods and filter out. 
            return MetadataOnlyModule.GetMethodImplHelper(this, name, bindingAttr, binder, callConvention, types, modifiers);
        }

        private Dictionary<BindingFlags, MemberInfo[]> _membersCache = new Dictionary<BindingFlags, MemberInfo[]>();

        public override MemberInfo[] GetMembers(System.Reflection.BindingFlags bindingAttr)
        {
            MemberInfo[] memberInfo = null;
            if (!_membersCache.TryGetValue(bindingAttr, out memberInfo))
            {
                memberInfo = MetadataOnlyTypeDef.GetMembersHelper(this, bindingAttr);
                _membersCache.Add(bindingAttr, memberInfo);
            }
            return memberInfo;
        }

        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return MetadataOnlyModule.GetConstructorOnType(this, bindingAttr, binder, callConvention, types, modifiers);
        }
        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            return MetadataOnlyModule.GetConstructorsOnType(this, bindingAttr);
        }

        #region Common implementations of inherited methods
        public override Module Module
        {
            get
            {
                // Module is a base class of a LMR resolver.
                return Resolver;
            }
        }

        /// <remarks>
        /// We can't rely on System.Type's default implementation since that assume object equality.  
        /// </remarks>
        public override bool Equals(object objOther)
        {
            Type t = objOther as Type;
            if (t == null)
                return false;

            // Chain to the Type based comparer.
            return this.Equals(t);
        }

        public override int GetHashCode()
        {
            // Use the metadata token (likely a TypeDef) as a default hash implementation. 
            // Some derived types that don't provide metadata tokens (arrays), can override.
            return this.MetadataToken;
        }

        /// <summary>
        /// Recursive check if this type has any unresolved generics.
        /// </summary>
        public override bool ContainsGenericParameters
        {
            get
            {
                // Need to get the root element type here, e.g. for (int*)[],
                // need to return int.
                if (HasElementType)
                    return GetRootElementType().ContainsGenericParameters;

                if (IsGenericParameter)
                    return true;

                if (!IsGenericType)
                    return false;

                Type[] genericArguments = GetGenericArguments();
                for (int i = 0; i < genericArguments.Length; i++)
                {
                    if (genericArguments[i].ContainsGenericParameters)
                        return true;
                }

                return false;
            }
        }

        private Type GetRootElementType()
        {
            Type rootElementType = this;

            while (rootElementType.HasElementType)
                rootElementType = rootElementType.GetElementType();

            return rootElementType;
        }

        /// <summary>
        /// Determines if this type is a subclass of a given type.
        /// </summary>
        /// <remarks>We can't rely on Reflection's implementation of
        /// this method since it uses reference equality (operator ==).</remarks>
        public override bool IsSubclassOf(Type c)
        {
            Type p = this;
            if (p.Equals(c))
                return false;
            while (p != null)
            {
                if (p.Equals(c))
                    return true;
                p = p.BaseType;
            }
            return false;
        }

        protected override bool IsContextfulImpl()
        {
            Type contextType = this.Resolver.AssemblyResolver.GetTypeXFromName("System.ContextBoundObject");
            if (contextType != null)
            {
                return contextType.IsAssignableFrom(this);
            }
            return false;
        }

        protected override bool IsMarshalByRefImpl()
        {
            Type marshalByRefType = this.Resolver.AssemblyResolver.GetTypeXFromName("System.MarshalByRefObject");
            if (marshalByRefType != null)
            {
                return marshalByRefType.IsAssignableFrom(this);
            }
            return false;
        }

        public override MemberInfo[] GetDefaultMembers()
        {
            Type defaultMemberAttrType = this.Resolver.AssemblyResolver.GetTypeXFromName("System.Reflection.DefaultMemberAttribute");
            if (defaultMemberAttrType == null)
            {
                return new MemberInfo[0];
            }

            CustomAttributeData attr = null;
            //Get the members with the DefaultMemberAttribute custom attribute
            for (Type t = this; t != null; t = t.BaseType)
            {
                IList<CustomAttributeData> attrs = t.GetCustomAttributesData();
                for (int i = 0; i < attrs.Count; i++)
                {
                    if (attrs[i].Constructor.DeclaringType.Equals(defaultMemberAttrType))
                    {
                        attr = attrs[i];
                        break;
                    }
                }

                if (attr != null)
                    break;
            }

            if (attr == null)
                return new MemberInfo[0];
            string defaultMember = attr.ConstructorArguments[0].Value as string;

            MemberInfo[] members = this.GetMember(defaultMember);
            if (members == null)
                members = new MemberInfo[0];
            return members;
        }

        public override bool IsInstanceOfType(object o)
        {
            return false;
        }


        public override string AssemblyQualifiedName
        {
            get
            {
                string t = this.FullName;
                // If the type's full name is null, return null for this property,
                // e.g. for type variables or generic types containing uninitiated type
                // parameters. Reflection has the same behavior.
                if (t == null)
                    return null;

                Assembly a = this.Assembly;
                Debug.Assert(a != null);
                string aqn = a.GetName().ToString();

                return System.Reflection.Assembly.CreateQualifiedName(aqn, t);
            }
        }

        // This matches CLR's reflection behavior
        public override bool IsSerializable
        {
            get
            {
                return (((this.GetAttributeFlagsImpl() & System.Reflection.TypeAttributes.Serializable) != 0)
                    || QuickSerializationCastCheck());
            }
        }

        // This matches System.TypeQuickSerializationCastCheck, which is private.
        private bool QuickSerializationCastCheck()
        {
            // CLR's impl calls typeof() here. We need to fetch this from the universe.
            var u = Helpers.Universe(this);
            var tEnum = u.GetTypeXFromName("System.Enum");
            var tDelegate = u.GetTypeXFromName("System.Delegate");


            Type c = this.UnderlyingSystemType;
            while (c != null)
            {
                if (c.Equals(tEnum) || c.Equals(tDelegate))
                {
                    return true;
                }
                c = c.BaseType;
            }
            return false;
        }

        #endregion // Common implementations of inherited methods

        #region Empty implementations

        /// <summary>
        /// LMR types doesn't support ReflectedType.
        /// ReflectedType is not actually in the metadata. Instead, it tracks which Type this object was obtained from.
        /// Mostly makes sense for MethodInfos, PropertyInfos, etc.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1065:DoNotRaiseExceptionsInUnexpectedLocations")]
        public override Type ReflectedType
        {
            get { throw new NotSupportedException(); }
        }

        public override bool IsEnum
        {
            get { return false; }
        }

        protected override bool IsArrayImpl()
        {
            // Array type would be implemented with code:MetadataOnlyArrayType class.
            return false;
        }

        protected override bool IsByRefImpl()
        {
            return false;
        }

        protected override bool IsPointerImpl()
        {
            return false;
        }

        protected override bool IsPrimitiveImpl()
        {
            return false;
        }

        // Type base class already has a default impl of IsValueTypeImpl, which returns false.

        public override bool IsGenericType
        {
            get
            {
                return false;
            }
        }

        public override bool IsGenericParameter
        {
            get
            {
                return false;
            }
        }

        /// <remarks>
        /// Runtime type throws here for types that are not type variables.
        /// </remarks>
        public override MethodBase DeclaringMethod
        {
            get
            {
                throw new InvalidOperationException(Resources.ValidOnGenericParameterTypeOnly);
            }
        }

        protected override bool IsCOMObjectImpl()
        {
            // The reflection uses native code to implement this by checking
            // the method table of the type. LMR currently doesn't support this.

            throw new NotImplementedException();
        }

        public override StructLayoutAttribute StructLayoutAttribute
        {
            get
            {
                return null;
            }
        }

        #endregion // Empty implementations

        #region Type algebra operations
        // Implementation of Type Algebra operations which can create other types.
        // These should go through the factory
        // These should build up a type-tree to avoid resolution.

        public override int GetArrayRank()
        {
            throw new ArgumentException(Resources.OperationValidOnArrayTypeOnly);
        }
        public override Type MakeGenericType(Type[] argTypes)
        {
            throw new InvalidOperationException();
        }

        public override Type MakeByRefType()
        {
            return this.Resolver.Factory.CreateByRefType(this);
        }

        public override Type MakePointerType()
        {
            return this.Resolver.Factory.CreatePointerType(this);
        }

        public override Type MakeArrayType()
        {
            // This method returns a vector type.
            // A vector, which always has only one dimension, is 
            // not the same as a multidimensional array that happens
            // to have only one dimension. This method overload can 
            // only be used to create vector types, and it is the only
            // way to create a vector type.
            //
            return this.Resolver.Factory.CreateVectorType(this);
        }

        public override Type MakeArrayType(int rank)
        {
            // This method returns a multidimensional array type.
            // You cannot use this method overload to create a vector 
            // type; if rank is 1, this method overload returns a 
            // multidimensional array type that happens to have one 
            // dimension. Use the MakeArrayType() method overload 
            // to create vector types.            
            //
            return MakeArrayTypeHelper(rank);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2201:DoNotRaiseReservedExceptionTypes")]
        Type MakeArrayTypeHelper(int rank)
        {
            if (rank <= 0)
            {
                // This is an FxCop violation, but that's reflection's existing behavior.
                throw new IndexOutOfRangeException();
            }
            return this.Resolver.Factory.CreateArrayType(this, rank);
        }

        #endregion // Type algebra operations

        #region Static helpers

        // Internal helper for formatting type names for various ToString calls, such as Parameter, FieldInfo, 
        // and MethodInfo. This behavior matches CLR's reflection implementation. 
        internal static string TypeSigToString(Type pThis)
        {
            StringBuilder sb = StringBuilderPool.Get();
            TypeSigToString(pThis, sb);

            string result = sb.ToString();
            StringBuilderPool.Release(ref sb);
            return result;
        }

        internal static void TypeSigToString(Type pThis, StringBuilder sb)
        {
            Type elementType = pThis;

            while (elementType.HasElementType)
                elementType = elementType.GetElementType();

            if (elementType.IsNested)
            {
                sb.Append(pThis.Name);
                return;
            }

            string sigToString = pThis.ToString();

            // To be fully correct, we should get System.Void from the universe rather than just compare the type name.
            //If the type is primitive or void, trim the namespace.
            if (elementType.IsPrimitive || (elementType.FullName == "System.Void"))
                sigToString = sigToString.Substring(@"System.".Length);

            sb.Append(sigToString);
        }

        #endregion // Static helpers
    }

} // namespace
