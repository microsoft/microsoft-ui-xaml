// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Default factory
// 

using System;
using System.Reflection.Adds;
using System.Reflection.Metadata;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using BindingFlags = System.Reflection.BindingFlags;

using System.Reflection;  
using Type = System.Type;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    // Default factory for creating LMR types. 
    internal class DefaultFactory : IReflectionFactory
    {
        #region IReflectionFactory Members

        public virtual MetadataOnlyCommonType CreateSimpleType(MetadataOnlyModule scope, TypeDefinitionHandle typeDef)
        {
            return new MetadataOnlyTypeDef(scope, typeDef);
        }

        public virtual MetadataOnlyCommonType CreateGenericType(MetadataOnlyModule scope, TypeDefinitionHandle typeDef, Type[] typeArgs)
        {
            return new MetadataOnlyTypeDef(scope, typeDef, typeArgs);
        }

        public virtual MetadataOnlyCommonType CreateArrayType(MetadataOnlyCommonType elementType, int rank)
        {
            return new MetadataOnlyArrayType(elementType, rank);
        }

        public virtual MetadataOnlyCommonType CreateVectorType(MetadataOnlyCommonType elementType) 
        {
            return new MetadataOnlyVectorType(elementType);
        }

        public virtual MetadataOnlyCommonType CreateByRefType(MetadataOnlyCommonType type)
        {
            return new MetadataOnlyModifiedType(type, "&");
        }

        public virtual MetadataOnlyCommonType CreatePointerType(MetadataOnlyCommonType type)
        {
            return new MetadataOnlyModifiedType(type, "*");
        }

        public virtual MetadataOnlyTypeVariable CreateTypeVariable(MetadataOnlyModule resolver, GenericParameterHandle genericParam)
        {
            return new MetadataOnlyTypeVariable(resolver, genericParam);
        }

        public virtual MetadataOnlyFieldInfo CreateField(MetadataOnlyModule resolver, FieldDefinitionHandle fieldDef, Type[] typeArgs, Type[] methodArgs)
        {
            return new MetadataOnlyFieldInfo(resolver, fieldDef, typeArgs, methodArgs);
        }

        public virtual MetadataOnlyPropertyInfo CreatePropertyInfo(MetadataOnlyModule resolver, PropertyDefinitionHandle propDef, Type[] typeArgs, Type[] methodArgs)
        {
            return new MetadataOnlyPropertyInfo(resolver, propDef, typeArgs, methodArgs);
        }

        public virtual MetadataOnlyEventInfo CreateEventInfo(MetadataOnlyModule resolver, EventDefinitionHandle eventDef, Type[] typeArgs, Type[] methodArgs)
        {
            return new MetadataOnlyEventInfo(resolver, eventDef, typeArgs, methodArgs);
        }


        #region Method Creation
        /// <summary>
        /// Create a constructor info around the given method
        /// </summary>
        /// <param name="method">method for the constructor</param>
        /// <returns>a constructor info for the given method</returns>
        public virtual MetadataOnlyConstructorInfo CreateConstructorInfo(MethodBase method)
        {
            return new MetadataOnlyConstructorInfo(method);
        }

        /// <summary>
        /// Create a MethodInfo for the given method. 
        /// </summary>
        /// <param name="method">method to create</param>
        /// <returns>can return method directly, or create a new wrapper around it.</returns>
        public virtual MetadataOnlyMethodInfo CreateMethodInfo(MetadataOnlyMethodInfo method)
        {
            return new MetadataOnlyMethodInfo(method);
        }

        // Default implementation to create a method or constructor.
        // This will chain to more specific CreateMethod/CreateConstructor callbacks.
        public virtual MethodBase CreateMethodOrConstructor(MetadataOnlyModule resolver, MethodDefinitionHandle methodDef, Type[] typeArgs, Type[] methodArgs)
        {
            // If this is a constructor, we need to instantiate a ConstructorInfo instead to be consistent with m.IsConstructor.
            MetadataOnlyMethodInfo m = new MetadataOnlyMethodInfo(resolver, methodDef, typeArgs, methodArgs);

            if (IsRawConstructor(m))
            {
                MetadataOnlyConstructorInfo ci = this.CreateConstructorInfo(m);
                Debug.Assert(ci is ConstructorInfo);
                return ci;
            }
            else
            {
                MetadataOnlyMethodInfo mi = this.CreateMethodInfo(m);
                Debug.Assert(mi is MethodInfo);
                return mi;
            }
        }
        // Return true iff the method info is for a constructor.
        // LMR must wrap this MethodInfo in a ConstructorMethodInfo before returning it to the user.
        static private bool IsRawConstructor(MethodInfo m)
        {
            // Constructors have a special name
            if ((m.Attributes & System.Reflection.MethodAttributes.RTSpecialName) == 0)
            {
                return false;
            }

            // Check name for ctor or static ctor match.
            string name = m.Name;
            if (name.Equals(System.Reflection.ConstructorInfo.ConstructorName, StringComparison.Ordinal))
            {
                return true;
            }

            if (name.Equals(System.Reflection.ConstructorInfo.TypeConstructorName, StringComparison.Ordinal))
            {
                return true;
            }

            return false;
        }
        #endregion // Method Creation


        public virtual bool TryCreateMethodBody(MetadataOnlyMethodInfo method, ref MethodBody body)
        {
            // Specify that we're not override methodbody creation.
            return false;
        }


        public virtual Type CreateTypeRef(MetadataOnlyModule scope, TypeReferenceHandle typeRef)
        {   
            return new MetadataOnlyTypeReference(scope, typeRef);            
        }

        public virtual Type CreateSignatureTypeRef(MetadataOnlyModule scope, TypeReferenceHandle typeRef, byte rawTypeKind)
        {
            return new MetadataOnlySignatureTypeReference(scope, typeRef, rawTypeKind);
        }

        public virtual Type CreateTypeSpec(MetadataOnlyModule scope, TypeSpecificationHandle typeSpec, Type[] typeArgs, Type[] methodArgs)
        {
            return new TypeSpec(scope, typeSpec, typeArgs, methodArgs);
        }

        #endregion
    } // end class DefaultFactory
}
