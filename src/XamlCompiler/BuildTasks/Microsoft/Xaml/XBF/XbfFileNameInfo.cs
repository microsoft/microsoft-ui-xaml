// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    internal class XbfFileNameInfo : IXbfFileNameInfo
    {
        public XbfFileNameInfo(string sourceXamlFullName, string givenXaml, string inputXaml, string outputXbf, string checksum = null)
        {
            this.GivenXamlName = givenXaml;
            this.InputXamlName = inputXaml;
            this.OutputXbfName = outputXbf;
            this.XamlFileChecksum = checksum ?? Utilities.ChecksumHelper.Instance.ComputeCheckSumForXamlFile(sourceXamlFullName);
        }

        // the "Given" name is used for error messages.
        public string GivenXamlName { get; set; }
        public string InputXamlName { get; set; }
        public string OutputXbfName { get; set; }
        public string XamlFileChecksum { get; set; }
    }
}