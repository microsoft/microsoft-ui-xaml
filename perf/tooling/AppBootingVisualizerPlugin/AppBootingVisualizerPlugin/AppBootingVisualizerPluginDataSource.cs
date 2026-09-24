using Microsoft.Performance.SDK.Processing;
using System;
using System.Collections.Generic;
using System.IO;

namespace AppBootingVisualizerPlugin
{
    [ProcessingSource(
        "{525EBCCB-8E5E-4BA5-961C-279B299CE2AE}",
        "App Booting Visualizer Plugin",
        "Analyzes application booting performance from ETL traces")]
    [FileDataSource(".etl", "ETL Trace Files")]
    public sealed class AppBootingVisualizerPluginDataSource : ProcessingSource
    {
        public AppBootingVisualizerPluginDataSource()
            : base()
        {
        }

        protected override bool IsDataSourceSupportedCore(IDataSource dataSource)
        {
            // Accept all .etl files
            return dataSource is FileDataSource fileDataSource 
                && Path.GetExtension(fileDataSource.FullPath).Equals(".etl", StringComparison.OrdinalIgnoreCase);
        }

        protected override ICustomDataProcessor CreateProcessorCore(
            IEnumerable<IDataSource> dataSources, 
            IProcessorEnvironment processorEnvironment,
            ProcessorOptions options)
        {
            return new AppBootingVisualizerPluginProcessor(dataSources);
        }

        public override ProcessingSourceInfo GetAboutInfo()
        {
            return new ProcessingSourceInfo
            {
                Owners = new[]
                {
                    new ContactInfo
                    {
                        Name = "Microsoft",
                        Address = string.Empty,
                        EmailAddresses = Array.Empty<string>(),
                    },
                },
                LicenseInfo = null,
                ProjectInfo = null,
                CopyrightNotice = $"Copyright (C) {DateTime.Now.Year}",
                AdditionalInformation = null,
            };
        }
    }
}
