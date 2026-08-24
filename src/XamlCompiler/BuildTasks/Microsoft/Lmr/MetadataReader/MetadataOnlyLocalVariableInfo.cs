// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// LocalVariableInfo
// 

using System;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;
using System.Collections.Generic;

using System.Reflection;  

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    internal class MetadataOnlyLocalVariableInfo : LocalVariableInfo
    {
        // Type of the local variable.
        readonly private Type m_type;

        // IL index of the local variable.
        readonly private int m_index;

        //Whether this is pinned
        readonly private bool m_fPinned;

        public MetadataOnlyLocalVariableInfo(int index, Type type, bool fPinned)
        {
            m_type = type;
            m_index = index;
            m_fPinned = fPinned;
        }

        public override bool IsPinned
        {
            get { return m_fPinned; }
        }

        public override int LocalIndex
        {
            get { return m_index; }
        }

        public override Type LocalType
        {
            get { return m_type; }
        }
    }
}