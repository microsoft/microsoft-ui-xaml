// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM.Visitors;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;

namespace OM
{
    public class OMContext
    {
        public const string DefaultImplementationNamespace = "DirectUI";

        private List<NamespaceDefinition> _namespaces = new List<NamespaceDefinition>();
        private List<Tuple<string, IEnumerable<TypeReference>>> _declareSections = new List<Tuple<string, IEnumerable<TypeReference>>>();

        public List<Tuple<string, IEnumerable<TypeReference>>> DeclareSections
        {
            get
            {
                return _declareSections;
            }
        }

        public ModuleDefinition Module
        {
            get;
            set;
        }

        public List<NamespaceDefinition> Namespaces
        {
            get
            {
                return _namespaces;
            }
        }

        public OMContextView GetView()
        {
            return new OMContextView(this);
        }

        public OMContextView GetView(Func<NamespaceDefinition, bool> nsFilter, string idlGroupName = null)
        {
            return new OMContextView(this, Namespaces.Where(nsFilter), idlGroupName);
        }

        public OMContextView GetView(Func<TypeDefinition, bool> typeFilter, string idlGroupName = null)
        {
            List<NamespaceDefinition> filteredNamespaces = new List<NamespaceDefinition>();
            foreach (NamespaceDefinition ns in Namespaces)
            {
                NamespaceDefinition nsView = new NamespaceDefinition(ns);
                nsView.Attributes.AddRange(ns.Attributes.Where(typeFilter).Cast<AttributeDefinition>());
                nsView.Classes.AddRange(ns.Classes.Where(typeFilter).Cast<ClassDefinition>());
                nsView.Enums.AddRange(ns.Enums.Where(typeFilter).Cast<EnumDefinition>());
                nsView.Delegates.AddRange(ns.Delegates.Where(typeFilter).Cast<DelegateDefinition>());
                nsView.Contracts.AddRange(ns.Contracts.Where(typeFilter).Cast<ContractDefinition>());
                filteredNamespaces.Add(nsView);
            }
            return new OMContextView(this, filteredNamespaces, idlGroupName);
        }

        public OMContextView GetView(Func<NamespaceDefinition, bool> nsFilter, Func<TypeDefinition, bool> typeFilter, string idlGroupName = null)
        {
            List<NamespaceDefinition> filteredNamespaces = new List<NamespaceDefinition>();
            foreach (NamespaceDefinition ns in Namespaces.Where(nsFilter))
            {
                NamespaceDefinition nsView = new NamespaceDefinition(ns);
                nsView.Attributes.AddRange(ns.Attributes.Where(typeFilter).Cast<AttributeDefinition>());
                nsView.Classes.AddRange(ns.Classes.Where(typeFilter).Cast<ClassDefinition>());
                nsView.Enums.AddRange(ns.Enums.Where(typeFilter).Cast<EnumDefinition>());
                nsView.Delegates.AddRange(ns.Delegates.Where(typeFilter).Cast<DelegateDefinition>());
                nsView.Contracts.AddRange(ns.Contracts.Where(typeFilter).Cast<ContractDefinition>());
                filteredNamespaces.Add(nsView);
            }
            return new OMContextView(this, filteredNamespaces, idlGroupName);
        }

        public byte[] ComputeHash()
        {
            using (MemoryStream stream = new MemoryStream())
            {
                HashVisitor.Hash(GetView(), stream);
                stream.Position = 0;

                SHA256 hash = SHA256.Create();
                return hash.ComputeHash(stream);
            }
        }
    }
}
