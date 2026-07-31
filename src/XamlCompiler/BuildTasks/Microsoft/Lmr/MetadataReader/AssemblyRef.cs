// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection.Adds;
    using Debug = Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
    using System.IO;

    using System.Reflection;  


    /// <summary>
    /// An Assembly proxy object given just the assembly name.
    /// </summary>
    /// <remarks>This is just a container for AssemblyName. 
    /// Ideally, this would also implement GetType(string,...) and hand out TypeRefs, and be consumed by the TypeNameParser.
    /// </remarks>
    [DebuggerDisplay("AssemblyRef: {m_name}")]
    internal class AssemblyRef : AssemblyProxy
    {
        /// <summary>
        /// The assembly name that this assembly is for.
        /// </summary>
        readonly AssemblyName m_name;

        public AssemblyRef(AssemblyName name, ITypeUniverse universe)
            : base(universe)
        {
            m_name = name;
            Debug.Assert(m_name != null);
        }
                
        protected override Assembly GetResolvedAssemblyWorker()
        {
            // We can't gaurantee that this will resolve to a LMR implementation. 
            // So we can't promise that GetType() results will be from LMR either.
            return this.TypeUniverse.ResolveAssembly(this.m_name);
        }

        protected override AssemblyName GetNameWithNoResolution()
        {
            return m_name;
        }

        // Inherited from Assembly.GetName(). 
        // Implement here to avoid resolution. 
        public override AssemblyName GetName()
        {
            return m_name;
        }
    }
}