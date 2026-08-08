// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Reflection.Metadata;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Reflection.Adds;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Represent a custom attribute. ConstructorInfo is eagerly fetched here since that's usually what
    /// a caller wants when the fetch the custom attributes. The argument blob (for typed and named
    /// arguments) is lazily fetched. Many code paths will just check the presence of an attribute, and so
    /// just need the constructor info. 
    /// </summary>
    internal class MetadataOnlyCustomAttributeData : CustomAttributeData
    {
        // Constructor info, which can provide the name of the attribute.
        readonly private ConstructorInfo m_ctor;
       
        // Module that handle is valid in.
        readonly private MetadataOnlyModule m_module;
        readonly private CustomAttributeHandle m_handle;

        // Cache of parse result from blob. 
        private IList<CustomAttributeTypedArgument> m_typedArguments;
        private IList<CustomAttributeNamedArgument> m_namedArguments;

        /// <summary>
        /// Constructor used when attribute info is obtained by parsing CA blobs. 
        /// </summary>
        /// <param name="module">module that the custom attribute handle is valid in. This will be the module
        /// containing the member that the attribute is on. </param>
        /// <param name="handle">custom attribute handle.</param>
        /// <param name="ctor">constructor for attributer. This provides the name of the attribute.</param>
        public MetadataOnlyCustomAttributeData(MetadataOnlyModule module, CustomAttributeHandle handle, ConstructorInfo ctor) 
        {
            m_ctor = ctor;
            m_handle = handle;
            m_module = module;
        }

        /// <summary>
        /// Constructor used for pseudo-custom attributes.
        /// </summary>
        /// <param name="ctor">Attribute's constructor.</param>
        /// <param name="typedArguments">List of attribute's typed arguments. Can't be null.</param>
        /// <param name="namedArguments">List of attribute's named arguments. Can't be null.</param>
        public MetadataOnlyCustomAttributeData(
            ConstructorInfo ctor, 
            IList<CustomAttributeTypedArgument> typedArguments,
            IList<CustomAttributeNamedArgument> namedArguments)
        {
            Debug.Assert(ctor != null, "Constructor parameter can't be null.");
            Debug.Assert(typedArguments != null, "Typed arguments parameter can't be null.");
            Debug.Assert(namedArguments != null, "Named arguments parameter can't be null.");

            m_ctor = ctor;
            m_typedArguments = typedArguments;
            m_namedArguments = namedArguments;
        }

        public override ConstructorInfo Constructor
        {
            get
            {
                return this.m_ctor;
            }
        }


        // Lazily initialize 
        private void InitArgumentData()
        {
            IList<CustomAttributeTypedArgument> typedArguments;
            IList<CustomAttributeNamedArgument> namedArguments;

            m_module.LazyAttributeParse(m_handle, m_ctor, out typedArguments, out namedArguments);

            // Now assign to fields. 
            m_typedArguments = typedArguments;
            m_namedArguments = namedArguments;
        }

        public override IList<CustomAttributeTypedArgument> ConstructorArguments
        {
            get
            {
                if (this.m_typedArguments == null)
                {
                    InitArgumentData();
                }
                return this.m_typedArguments;
            }
        }

        public override IList<CustomAttributeNamedArgument> NamedArguments
        {
            get
            {
                if (m_namedArguments == null)
                {
                    InitArgumentData();
                }
                return this.m_namedArguments;
            }
        }
    }
}
