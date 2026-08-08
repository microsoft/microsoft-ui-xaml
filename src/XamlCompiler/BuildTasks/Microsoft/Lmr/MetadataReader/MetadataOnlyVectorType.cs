// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection.Adds;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// This class represents vector types.
    /// The CLR makes a distinction between vectors (that is, one-dimensional 
    /// arrays that are always zero-based) and multidimensional arrays. 
    /// A vector, which always has only one dimension, is not the same
    /// as a multidimensional array that happens to have only one dimension.
    /// You cannot use this method overload to create a vector type; if rank
    /// is 1, this method overload returns a multidimensional array type that
    /// happens to have one dimension.
    /// </summary>
    internal class MetadataOnlyVectorType : MetadataOnlyCommonArrayType
    {
        public MetadataOnlyVectorType(MetadataOnlyCommonType elementType)
            : base(elementType)
        {
        }

        public override string FullName
        {
            get {
                // For a type-variable T, element name is null. In that case, return null.
                // If the element type is a generic type definition, return null. This is what
                // the reflection does.
                string elementName = this.GetElementType().FullName;
                if (elementName == null || this.GetElementType().IsGenericTypeDefinition)
                    return null;

                return elementName + "[]";
            }
        }

        public override int GetArrayRank()
        {
            // Vectors is always 1d.
            return 1;
        }

        protected override bool IsArrayImpl()
        {
            return true;
        }

        public override bool Equals(Type t) {
            if (t == null) {
                return false;
            }

            // Vectors are not equal to 1d array created by Type.MakeArrayType(1).
            // The only difference between vector types and 1d arrays are their ToString
            // representation. Vectors are displayed as "T[]", and multi-dimensional
            // arrays with one dimension are displayed as "T[*]".
            // However, we don't want to rely on the string representation when comparing
            // the types, so we decide to require the other side to be a LMR vector type to
            // make it equal. This will disallow a LMR vector type to be equal to a non-LMR
            // type.
            return t is MetadataOnlyVectorType &&
                   t.GetArrayRank() == 1 &&
                   this.GetElementType().Equals(t.GetElementType());
        }


        public override bool IsAssignableFrom(Type c)
        {
            if (c == null)
            {
                return false;
            }

            if (c.IsArray && c.GetArrayRank() == 1 && 
                c is MetadataOnlyVectorType)
            {
                // Vectors can only be assigned from vectors, not 1d arrays.

                //If the IsAssignableFrom relation holds for the element types 
                // and the element type of c is not value type, the relation also
                // holds for the array types. When c's element type is a value type,
                // it must be equal to the element type of the current array type.
                //e.g. typeof(object[]).IsAssignableFrom(typeof(string[])) is true
                //e.g. typeof(ValueType[]).IsAssignableFrom(typeof(int[])) is false
                Type elemType = c.GetElementType();
                if (elemType.IsValueType) 
                {
                    return this.GetElementType().Equals(elemType);
                } 
                else 
                {
                    return this.GetElementType().IsAssignableFrom(elemType);
                }
            }
            return MetadataOnlyTypeDef.IsAssignableFromHelper(this, c);

        }

        public override string Name
        {
            get 
            {
                return this.GetElementType().Name + "[]";
            }
        }

        public override string ToString()
        {
            return this.GetElementType().ToString() + "[]";
        }

    }
}
