// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection.Adds;
using System.Diagnostics;
using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;


using MethodAttributes = System.Reflection.MethodAttributes;
using CallingConventions = System.Reflection.CallingConventions;
using MethodImplAttributes = System.Reflection.MethodImplAttributes;
using MemberTypes = System.Reflection.MemberTypes;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Represent the fabricated constructors on an array class. 
    /// See Ecma IIb 14.2.
    /// Each array has 2 constructors which just take a vector of integers for the parameter.
    /// One ctor has an Int32 parameter for each dimension.
    /// The other ctor has 2 Int32 parameters for each dimension
    /// </summary>
    /// <remarks>
    /// Reflection requires constructors derive from ConstructorInfo. However, we want to share
    /// extensive functionality with the fabricated methods. 
    /// So we have an internal Adapter that uses the array fabricated MethodInfo support,
    /// and then we publicly wrap it in a ConstructorInfo for public consumption.
    /// </remarks>
    class ArrayFabricatedConstructorInfo : MetadataOnlyConstructorInfo
    {
        readonly int m_numParams;

        /// <summary>
        /// Ctor for array fabricated constructor.
        /// </summary>
        /// <param name="arrayType">The declaring type that the ctor is on. arrayType.IsArrayType must be true.</param>
        /// <param name="numParams">the number of Int32 parameters in the ctor's signature.</param>
        public ArrayFabricatedConstructorInfo(Type arrayType, int numParams)
            : base(MakeMethodInfo(arrayType, numParams))
        {
            m_numParams = numParams;
        }
        
        // Create a MethodInfo that represents the fabricated constructor.
        static MethodInfo MakeMethodInfo(Type arrayType, int numParams)
        {
            return new Adapter(arrayType, numParams);
        }

        public override bool Equals(object obj)
        {
            ArrayFabricatedConstructorInfo other = obj as ArrayFabricatedConstructorInfo;
            if (other == null)
                return false;


            if (!this.DeclaringType.Equals(other.DeclaringType))
            {
                return false;
            }

            // Compare signatures
            return this.ToString().Equals(other.ToString());
        }

        public override int GetHashCode()
        {
            return this.DeclaringType.GetHashCode() + m_numParams;
        }

        /// <summary>
        /// To make it easier for LMR clients that extend LMR objects, we directly
        /// return empty arrays here so they don't need to go through factory.
        /// </summary>
        public override object[] GetCustomAttributes(bool inherit)
        {
            return new object[0];
        }

        /// <summary>
        /// To make it easier for LMR clients that extend LMR objects, we directly
        /// return empty arrays here so they don't need to go through factory.
        /// </summary>
        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return new object[0];
        }

        /// <summary>
        /// Internal Adapter to provide functionality using ArrayFabricatedMethodInfo.
        /// This is then publicly wrapped in a constructorInfo to match the reflection construct. 
        /// </summary>
        class Adapter : ArrayFabricatedMethodInfo
        {
            // Number of Int32 to appear in the signature.
            readonly int m_numParams;

            // Parameters here are from containing class's ctor.
            public Adapter(Type arrayType, int numParams) : base(arrayType)
            {
                m_numParams = numParams;
            }

            public override string Name
            {
                get { return ".ctor"; }
            }

            // Constructor's paremeters are just a set of Int32. 
            public override ParameterInfo[] GetParameters()
            {
                ITypeUniverse u = this.Universe;
                Type tInt = u.GetBuiltInType(CorElementType.Int);

                ParameterInfo[] p = new ParameterInfo[m_numParams];

                for (int i = 0; i < m_numParams; i++)
                {
                    p[i] = MakeParameterInfo(tInt, i);
                }
                return p;
            }

            public override MethodAttributes Attributes
            {
                get
                {
                    // This isn't spec'ed, but it's what reflection returns. 
                    // RTSpecialName is important because constructors have a special name, .ctor.
                    return MethodAttributes.Public | MethodAttributes.RTSpecialName;
                }
            }

            // ConstructorInfos don't have return types.
            // However, the underlying metadata representation is a method with a void return type.
            // Public callers shouldn't be able to get here directly, but some internal facilities (like ToString()) 
            // can get here.
            public override Type ReturnType
            {
                get {
                    return this.Universe.GetBuiltInType(CorElementType.Void);                    
                }
            }



        }
    }


    /// <summary>
    /// MethodInfo represented the fabricated method for an Array getter.
    /// Signature is:
    ///     T Get(...)
    /// </summary>
    class ArrayFabricatedGetMethodInfo : ArrayFabricatedMethodInfo
    {
        public ArrayFabricatedGetMethodInfo(Type arrayType)
            : base(arrayType)
        {
        }

        public override string Name
        {
            get { return "Get"; }
        }

        public override ParameterInfo[] GetParameters()
        {
            return MakeParameterHelper(0);
        }

        public override Type ReturnType
        {
            get { return this.GetElementType(); }
        }
    }


    /// <summary>
    /// MethodInfo represented the fabricated method for an Array setter.
    /// Signature is:
    ///     void Set(..., T)
    /// </summary>
    class ArrayFabricatedSetMethodInfo : ArrayFabricatedMethodInfo
    {
        public ArrayFabricatedSetMethodInfo(Type arrayType)
            : base(arrayType)
        {
        }

        public override string Name
        {
            get { return "Set"; }
        }

        public override ParameterInfo[] GetParameters()
        {
            const int extra = 1; // space for element type parameter at end.
            ParameterInfo[] p = MakeParameterHelper(extra);

            // Append element type parameter to the end of the list.
            int position = Rank;
            Type t = this.GetElementType();
            p[position] = MakeParameterInfo(t, position);

            return p;
        }

        public override Type ReturnType
        {
            get { return this.Universe.GetBuiltInType(CorElementType.Void); }
        }
    }

    /// <summary>
    /// MethodInfo represented the fabricated method for an Array address operator.
    /// Signature is:
    ///    T& Address(...)
    /// </summary>    
    class ArrayFabricatedAddressMethodInfo : ArrayFabricatedMethodInfo
    {
        public ArrayFabricatedAddressMethodInfo(Type arrayType)
            : base(arrayType)
        {
        }

        public override string Name
        {
            get { return "Address"; }
        }

        public override ParameterInfo[] GetParameters()
        {
            return MakeParameterHelper(0);
        }

        public override Type ReturnType
        {
            get { return this.GetElementType().MakeByRefType(); }
        }
    }


    
    
    /// <summary>
    /// Base class for MethodInfos for fabricated methods on an array (Get/Set/Address).
    /// Each specific method can have its own derived class. This avoids having to eagerly populate
    /// all the properties and facilates quickly creating tearoff objects with lazy properties. 
    /// 
    /// The MethodInfo's signatures are a function of the declaring type's Array rank and element type.
    ///  "..." represents a set of parameters, one int32 for each dimension in the array. This depends on the rank.
    ///  "T" represents the declaring type's element type.
    ///  
    /// You can see this codepath by inspecting something like:
    ///    elementType.MakeArrayType(n).GetMethods(BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public).
    /// </summary>
    abstract class ArrayFabricatedMethodInfo : MethodInfo    
    {
        // The array type that this is declared on. We get the rank and element type from this.
        Type m_arrayType;

        #region Helpers
        /// <summary>
        /// Protected constructor for derived classes.
        /// </summary>
        /// <param name="arrayType"></param>
        protected ArrayFabricatedMethodInfo(Type arrayType)
        {
            Debug.Assert(arrayType != null);
            m_arrayType = arrayType;


        }

        // Convenience Helper to get the type universe that this method lives in.
        // This is useful for creating the types used in the parameters.
        protected ITypeUniverse Universe
        {
            get
            {
                return Helpers.Universe(m_arrayType);
            }
        }

        // Convenience Helper to get rank for derived classes.
        protected int Rank
        {
            get { return m_arrayType.GetArrayRank(); }
        }

        // Convenience Helper to get the element type of the array.
        protected Type GetElementType()
        {
            return m_arrayType.GetElementType(); 
        }
                
        /// <summary>
        /// Helper to Get the fabricated parameters for the rank.
        /// See Ecma IIb 14.2 for details. This is an System.Int32 parameter for each rank in the array
        /// </summary>
        /// <param name="extra">Allocate extra slots at the end of the ParameterInfo array. 
        /// Some signatures (eg, Set(..., T)) have additional parameters. So we allow allocating extra space so 
        /// that the caller can just fill in the extra parameters without reallocating a new array.
        /// </param>
        /// <returns>an array of ParameterInfos of length (this.Rank + extra).
        /// </returns>
        protected ParameterInfo[] MakeParameterHelper(int extra)
        {
            Debug.Assert(extra >= 0);
            int rank = this.Rank;

            ITypeUniverse u = this.Universe;
            Type tInt = u.GetBuiltInType(CorElementType.Int);

            ParameterInfo[] p = new ParameterInfo[rank + extra];

            for (int i = 0; i < rank; i++)
            {
                p[i] = MakeParameterInfo(tInt, i);
            }
            return p;
        }

        // Helper to make a ParameterInfo around the given type and position.
        // Position == -1 for the 'return parameter'.
        protected ParameterInfo MakeParameterInfo(Type t, int position)
        {
            Debug.Assert(t != null);

            return new SimpleParameterInfo(this, t, position);
        }

        #endregion // Helpers


        public override System.Reflection.ICustomAttributeProvider ReturnTypeCustomAttributes
        {
            get { throw new NotImplementedException(); }
        }

        public override MethodInfo GetBaseDefinition()
        {
            // The fabricated methods aren't inherited, so this is the first time they show up
            // in the class hierarchy.
            return this;
        }

        public override ParameterInfo ReturnParameter
        {
            get { return MakeParameterInfo(this.ReturnType, -1); }
        }

        public override MethodAttributes Attributes
        {            
            get {
                return MethodAttributes.FamANDAssem | MethodAttributes.Family;
            }
        }

        public override CallingConventions CallingConvention
        {
            get {
                return CallingConventions.HasThis | CallingConventions.Standard;
            }
        }

        public override MethodInfo MakeGenericMethod(params Type[] types)
        {
            // IsGenericMethodDefinition is false, so we can't make a generic method of it.
            Debug.Assert(!IsGenericMethodDefinition);
            throw new InvalidOperationException();
        }

        public override bool IsGenericMethodDefinition
        {
            // This is never a generic method definition.
            // Even if tthe element type is generic, this is still not a generic.
            get { return false;  }
        }

        public override Type[] GetGenericArguments()
        {
            return Type.EmptyTypes;
        }

        public override bool ContainsGenericParameters
        {            
            get {
                return this.GetElementType().IsGenericParameter;
            }
        }

        public override MethodBody GetMethodBody()
        {
            // Array members don't have any
            return null; 
        }

        public override MethodImplAttributes GetMethodImplementationFlags()
        {
            // Empirically, reflection returns IL. This is the wrong value because these methods don't actually 
            // have any IL (GetMethodBody() returns null).
            return MethodImplAttributes.IL;
        }

        public override object Invoke(object obj, System.Reflection.BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture)
        {
            throw new NotSupportedException();
        }

        public override RuntimeMethodHandle MethodHandle
        {
            get { throw new NotSupportedException(); }
        }

        public override MemberTypes MemberType
        {
            get { return MemberTypes.Method; }
        }

        public override Type DeclaringType
        {
            // Declaring type is the initial array type that this method was fabricated for.
            get { return m_arrayType; }
        }


        public override int MetadataToken
        {
            // Reflection's behavior is to return a 0x06000000 token.
            get { return 0x06000000; }
        }

        public override Module Module
        {
            get {
                // Reflection behavior is that the Module is the same as the declaring type's.
                return this.DeclaringType.Module;
            }
        }

        public override Type ReflectedType
        {
            // LMR does not support ReflectedType property.
            get { throw new NotSupportedException(); }
        }

        /// <summary>
        /// To make it easier for LMR clients that extend LMR objects, we directly
        /// return empty arrays here so they don't need to go through factory.
        /// </summary>
        public override object[] GetCustomAttributes(bool inherit)
        {
            return new object[0];
        }

        /// <summary>
        /// To make it easier for LMR clients that extend LMR objects, we directly
        /// return empty arrays here so they don't need to go through factory.
        /// </summary>
        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return new object[0];
        }

        public override bool IsDefined(Type attributeType, bool inherit)
        {
            throw new NotImplementedException();
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            // Reflection does not have any custom attributes on the fabricated array methods. 
            return new CustomAttributeData[0];
        }

        public override string ToString()
        {         
            return MetadataOnlyMethodInfo.CommonToString(this);                    
        }

        // Equality check. 
        public override bool Equals(object obj)
        {
            ArrayFabricatedMethodInfo mi = obj as ArrayFabricatedMethodInfo;

            if (mi == null)
            {
                return false;
            }
          
            if (!this.DeclaringType.Equals(mi.DeclaringType))
            {
                return false;
            }

            // Compare 
            // Since other method must be an array fabricated MethodInfo, 
            // we know that it's uniquely described by (Declaring Type + Name)
            return (this.Name.Equals(mi.Name));
        }

        public override int GetHashCode()
        {
            return this.DeclaringType.GetHashCode() + this.Name.GetHashCode();
        }
    } // end class

}
