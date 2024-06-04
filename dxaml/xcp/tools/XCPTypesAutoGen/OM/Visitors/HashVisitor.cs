// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.IO;
using System.Linq;
using System.Reflection;

namespace OM.Visitors
{
    internal class HashVisitor : OMVisitor
    {
        private StreamWriter _writer;

        public HashVisitor(OMContextView view, StreamWriter writer)
            : base(view)
        {
            _writer = writer;
        }

        internal static void Hash(OMContextView view, Stream stream)
        {
            StreamWriter writer = new StreamWriter(stream);
            HashVisitor visitor = new HashVisitor(view, writer);
            visitor.Run();
            writer.Flush();
        }

        protected override void VisitNamespace(NamespaceDefinition ns)
        {
            base.VisitNamespace(ns);

            if (ns.Name != null)
            {
                _writer.Write(ns.Name.GetHashCode());
            }
        }

        protected override void VisitType(TypeDefinition type)
        {
            base.VisitType(type);
            ShallowSerializeProperties(type);
        }

        protected override void VisitTypeRef(TypeReference typeRef)
        {
            base.VisitTypeRef(typeRef);
            ShallowSerializeProperties(typeRef);
        }

        protected override void VisitClass(ClassDefinition c)
        {
            base.VisitClass(c);

            foreach (ClassVersion v in c.Versions)
            {
                ShallowSerializeProperties(v);
            }
        }

        protected override void VisitMember(MemberDefinition m)
        {
            base.VisitMember(m);
            ShallowSerializeProperties(m);
        }

        private void ShallowSerializeProperties(object obj)
        {
            foreach (PropertyInfo property in obj.GetType().GetProperties().Where(p => p.CanRead && p.CanWrite))
            {
                object value = property.GetValue(obj, null);
                if (value != null)
                {
                    _writer.Write(value.GetHashCode());
                }
            }
        }
    }
}
