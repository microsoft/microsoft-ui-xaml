// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public class XbfNode<T0> : XbfNode
    {
        public Tuple<T0> Values { get; }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset)
            : base(nodeType, nodeStreamOffset)
        {
        }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset, T0 value0)
            : base(nodeType, nodeStreamOffset)
        {
            Values = new Tuple<T0>(value0);
        }

        public override string ToString()
        {
            return $"{base.ToString()} {Values}";
        }
    }

    public class XbfNode<T0, T1> : XbfNode
    {
        public Tuple<T0, T1> Values { get; }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset)
            : base(nodeType, nodeStreamOffset)
        {
        }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset, T0 value0, T1 value1)
            : base(nodeType, nodeStreamOffset)
        {
            Values = new Tuple<T0, T1>(value0, value1);
        }

        public override string ToString()
        {
            return $"{base.ToString()} {Values}";
        }
    }

    public class XbfNode<T0, T1, T2> : XbfNode
    {
        public Tuple<T0, T1, T2> Values { get; }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset)
            : base(nodeType, nodeStreamOffset)
        {
        }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset, T0 value0, T1 value1, T2 value2)
            : base(nodeType, nodeStreamOffset)
        {
            Values = new Tuple<T0, T1, T2>(value0, value1, value2);
        }
        public override string ToString()
        {
            return $"{base.ToString()} {Values}";
        }
    }

    public class XbfNode<T0, T1, T2, T3> : XbfNode
    {
        public Tuple<T0, T1, T2, T3> Values { get; }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset)
            : base(nodeType, nodeStreamOffset)
        {
        }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset, T0 value0, T1 value1, T2 value2, T3 value3)
            : base(nodeType, nodeStreamOffset)
        {
            Values = new Tuple<T0, T1, T2, T3>(value0, value1, value2, value3);
        }

        public override string ToString()
        {
            return $"{base.ToString()} {Values}";
        }
    }

}
