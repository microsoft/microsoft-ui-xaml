// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    using System.IO;
    using System.Security.Cryptography;
    using System.Text;

    internal class ChecksumHelper
    {
        private static ChecksumHelper instance;

        // The checksum MUST be 64 chars.
        // It matches the OS code: os/onecoreuap/windows/dxaml/xcp/core/inc/XamlBinaryMetadata.h
        internal const int ChecksumLength = 64;

        internal static ChecksumHelper Instance
        {
            get
            {
                if (ChecksumHelper.instance == null)
                {
                    ChecksumHelper.instance = new ChecksumHelper();
                }

                return ChecksumHelper.instance;
            }
        }

        private HashAlgorithm hashProvider;

        private ChecksumHelper()
        {
            this.hashProvider = new SHA256CryptoServiceProvider();
        }

        internal string ComputeCheckSumForXamlFile(string fullXamlFilePath)
        {
            byte[] hashData = null;
            using (FileStream fileStream = File.Open(fullXamlFilePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                hashData = this.hashProvider.ComputeHash(fileStream);
            }

            StringBuilder checkSum = new StringBuilder(ChecksumHelper.ChecksumLength);
            for (int i = 0; checkSum.Length < ChecksumHelper.ChecksumLength && i < hashData.Length; i++)
            {
                checkSum.Append(hashData[i].ToString("X2"));
            }

            return checkSum.ToString();
        }
    }
}
