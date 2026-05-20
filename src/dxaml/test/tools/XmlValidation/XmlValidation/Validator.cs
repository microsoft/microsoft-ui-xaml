// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.IO;
using System.Xml;
using System.Xml.XPath;

namespace XmlValidation
{
    public class Validator
    {
        private Rules rules;
        private XPathDocument referenceDoc;
        private XPathDocument validatedDoc;

        private static XPathDocument CreateXPathDocument(string xml)
        {
            // Using XPathDocument(string filename) throws XmlException - internal error when run under ccrun
            using (TextReader reader = new StringReader(xml))
            {
                return new XPathDocument(XmlReader.Create(reader));
            }
        }

        public void LoadRulesFromFile(string file)
        {
            LoadRulesFromXml(File.ReadAllText(file));
        }

        public void LoadRulesFromXml(string rulesXml)
        {
            XPathDocument rulesDoc = CreateXPathDocument(rulesXml);
            XPathNavigator rulesNode = rulesDoc.CreateNavigator().SelectSingleNode("/Rules");

            rules = null;

            if (rulesNode != null)
            {
                rules = new Rules();
                rules.Load(rulesNode);
            }
        }

        private void CreateDefaultRules()
        {
            LoadRulesFromXml(
                @"<?xml version='1.0' encoding='UTF-8'?>
                <Rules>
                  <Rule Applicability='//Element' Inclusion='Blacklist'/>
                </Rules>");
        }

        public void LoadReference(ReferenceFile file)
        {
            referenceDoc = CreateXPathDocument(file.GetContent());
        }

        public void LoadValidated(ValidatedFile file)
        {
            validatedDoc = CreateXPathDocument(file.GetContent());
        }

        private void OutputChanges(TreeModifications treeChanges)
        {
            foreach (TreeModificationInfo tmi in treeChanges)
            {
                Console.Out.WriteLine(tmi);
            }
        }

        private bool ValidateSubtree(TreeStructureComparator tsc, TreePropertyComparator pc, string name)
        {
            string nodeName = string.Format("/{0}/{1}", Common.RootNodeName, name);

            // Precompute and cache tree information to speed up comparisons.
            TreeComparisonContext tcx = new TreeComparisonContext(
                referenceDoc.CreateNavigator().SelectSingleNode(nodeName),
                validatedDoc.CreateNavigator().SelectSingleNode(nodeName));

            // Compare structure of trees
            bool treeStructurePass = tsc.Compare(tcx);

            // Even if structures differ, make an effort to map nodes to compare properties
            pc.KnownTreeStructureDifferences = (!treeStructurePass) ? tsc.GetChanges(false) : null;

            // Compare properties
            bool propertyPass = pc.Compare(rules, tcx);

            return treeStructurePass && propertyPass;
        }

        public bool Validate()
        {
            if (referenceDoc == null ||
                validatedDoc == null)
            {
                throw new Exception("Required documents are not specified");
            }

            if (rules == null)
            {
                CreateDefaultRules();
            }

            TreeStructureComparator tsc = new TreeStructureComparator();
            TreePropertyComparator pc = new TreePropertyComparator();

            bool visualRootPass = ValidateSubtree(tsc, pc, Common.VisualRootNodeName);
            bool popupRootPass = ValidateSubtree(tsc, pc, Common.PopupRootNodeName);

            // Tree structure changes should be output before property changes
            OutputChanges(tsc.GetChanges());
            OutputChanges(pc.GetChanges());

            return visualRootPass && popupRootPass;
        }
    }
}