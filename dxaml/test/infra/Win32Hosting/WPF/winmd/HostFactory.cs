// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;

namespace Private.Infrastructure.Hosting.WPF
{
    [global::Windows.Foundation.Metadata.MarshalingBehavior(global::Windows.Foundation.Metadata.MarshalingType.Agile)]
    public sealed partial class HostFactory : IWin32HostFactory, IWin32HostFactorySystemCompositionEngine
    {
        private const int MaximumCompositionEngineLafTokenSize = 64 * 1024;
        private const int MaximumCompositionEngineLaunchRequestSize = 1024;
        private const int SharedMemoryHeaderSize = sizeof(int);
        private const long SharedMemoryCapacity =
            SharedMemoryHeaderSize + MaximumCompositionEngineLafTokenSize;
        private const string SharedMemoryNamePrefix =
            "WinUI.SystemCompositionEngine.";
        private bool useSystemCompositionEngine;
        private string compositionEngineLafToken;

        public void SetExceptionHandler(ExceptionHandler handler)
        {
            AppDomainExceptionHandler.SetExceptionHandler(handler);
        }

        public void ConfigureSystemCompositionEngine(string launchRequestPath)
        {
            if (string.IsNullOrEmpty(launchRequestPath))
            {
                throw new ArgumentException(
                    "System composition engine tests require a launch request path.",
                    nameof(launchRequestPath));
            }

            if (SystemCompositionEngineTestSupport.IsConfigured)
            {
                useSystemCompositionEngine = true;
                return;
            }

            string sharedMemoryName;
            using (var stream = new FileStream(
                launchRequestPath,
                FileMode.Open,
                FileAccess.Read,
                FileShare.Read))
            {
                if (stream.Length <= 0 ||
                    stream.Length > MaximumCompositionEngineLaunchRequestSize)
                {
                    throw new UnauthorizedAccessException(
                        "The system composition engine launch request is invalid.");
                }

                using (var reader = new StreamReader(
                    stream,
                    new UTF8Encoding(false, true),
                    false))
                {
                    sharedMemoryName = reader.ReadToEnd();
                }
            }
            if (string.IsNullOrEmpty(sharedMemoryName) ||
                !sharedMemoryName.StartsWith(
                    SharedMemoryNamePrefix,
                    StringComparison.Ordinal) ||
                sharedMemoryName.IndexOf('\0') >= 0)
            {
                throw new UnauthorizedAccessException(
                    "The system composition engine launch request is invalid.");
            }

            byte[] tokenBytes = null;
            try
            {
                using (var sharedMemory = MemoryMappedFile.OpenExisting(
                    sharedMemoryName,
                    MemoryMappedFileRights.Read))
                using (var view = sharedMemory.CreateViewAccessor(
                    0,
                    SharedMemoryCapacity,
                    MemoryMappedFileAccess.Read))
                {
                    var tokenSize = view.ReadInt32(0);
                    if (tokenSize <= 0 ||
                        tokenSize > MaximumCompositionEngineLafTokenSize)
                    {
                        throw new UnauthorizedAccessException(
                            "The system composition engine launch request is invalid.");
                    }

                    tokenBytes = new byte[tokenSize];
                    if (view.ReadArray(
                            SharedMemoryHeaderSize,
                            tokenBytes,
                            0,
                            tokenBytes.Length) != tokenBytes.Length)
                    {
                        throw new InvalidDataException(
                            "The system composition engine launch request could not be read.");
                    }

                    compositionEngineLafToken =
                        new UTF8Encoding(false, true).GetString(tokenBytes);
                }
            }
            finally
            {
                if (tokenBytes != null)
                {
                    Array.Clear(tokenBytes, 0, tokenBytes.Length);
                }
            }
            if (string.IsNullOrEmpty(compositionEngineLafToken) ||
                compositionEngineLafToken.IndexOf('\0') >= 0)
            {
                throw new UnauthorizedAccessException(
                    "The system composition engine launch request is empty.");
            }
            useSystemCompositionEngine = true;
        }

        private async Task<object> CreateInternal(DpiAwarenessContext dpiAwarenessContext, bool initCore)
        {
            var wpfHost = new WPFHost(
                dpiAwarenessContext,
                initCore,
                useSystemCompositionEngine,
                compositionEngineLafToken);
            await wpfHost.EnsureWindow();
            return wpfHost;
        }

        public IAsyncOperation<object> Create(DpiAwarenessContext dpiAwarenessContext, bool initCore)
        {
            return CreateInternal(dpiAwarenessContext, initCore).AsAsyncOperation();
        }
    }
}
