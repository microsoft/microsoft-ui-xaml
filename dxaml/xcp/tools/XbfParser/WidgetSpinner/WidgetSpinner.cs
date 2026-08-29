// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Model;
using Microsoft.Xaml.WidgetSpinner.Reader;
using Microsoft.Xaml.WidgetSpinner.Writer;
using Microsoft.Xaml.WidgetSpinner.XBF;
using System;
using System.IO;

namespace Microsoft.Xaml.WidgetSpinner
{
    /// <summary>
    /// Entry point class for XBF processing
    /// </summary>
    public class WidgetSpinner
    {
        /// <summary>
        /// Deserializes the specified XBF file
        /// </summary>
        /// <param name="xbfPath">Path to the file</param>
        /// <returns>An XbfFile object containing the deserialized data</returns>
        public XbfFile DeserializeXbfFile(string xbfPath)
        {
            try
            {
                using (Stream stream = File.OpenRead(xbfPath))
                {
                    return DeserializeXbfFile(stream);
                }
            }
            catch (Exception e)
            {
                ErrorService.Instance.RaiseError(e.Message);
                return null;
            }
        }

        /// <summary>
        /// Deserializes the specified XBF file
        /// </summary>
        /// <param name="stream">A Stream containing data in XBF format</param>
        /// <returns>An XbfFile object containing the deserialized data</returns>
        public XbfFile DeserializeXbfFile(Stream stream)
        {
            try
            {
                using (var reader = new XbfReader(stream))
                {
                    return reader.DeserializeXbfFile();
                }
            }
            catch (Exception e)
            {
                ErrorService.Instance.RaiseError(e.Message);
                return null;
            }
        }

        /// <summary>
        /// Creates a graph of XamlObjects from an XbfFile
        /// </summary>
        /// <returns>The root XamlObject of the object graph</returns>
        public XamlObject CreateObjectGraph(XbfFile xbfFile)
        {
            try
            {
                var objectWriter = new ObjectWriter(xbfFile);
                var result = objectWriter.ProcessXbfFile() as XamlObject;

                if (result == null)
                {
                    ErrorService.Instance.RaiseError("Expected XBF file processing to result in a XamlObject");
                }

                return result;
            }
            catch (Exception e)
            {
                ErrorService.Instance.RaiseError(e.Message);
                return null;
            }
        }
    }
}
