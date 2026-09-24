// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IWICImagingFactory;
struct IWICBitmapSource;

#include "ImageBuffer.h"
#include <map>
#include <string>

namespace Private {
    namespace Infrastructure {

        // Interface for talking to different verification methods.

        struct VerificationComparer
        {
            virtual ~VerificationComparer();

            virtual bool CompareFiles(
                _In_ const wrl::Wrappers::HStringReference&,
                _In_ const wrl::Wrappers::HStringReference&) = 0;

            virtual void OutputAdditionalResults(
                bool isXmlFile) = 0;
        };

        // Byte-by-byte comparison with diff output.
        // Supports $$variable$$ substitution in master files for OS-dependent values
        // (e.g. Surface CRC hashes, accent colors). Tests call SetDCompXmlVariable()
        // to set values; the comparer replaces $$name$$ placeholders before comparing.

        class ByteByByteComparer : public VerificationComparer
        {
        public:
            bool CompareFiles(
                _In_ const wrl::Wrappers::HStringReference& s1,
                _In_ const wrl::Wrappers::HStringReference& s2) override;

            void OutputAdditionalResults(
                bool isXmlFile) override;

            // Set variables for $$name$$ substitution in master files.
            void SetVariables(const std::map<std::wstring, std::wstring>& vars) { m_variables = vars; }

        private:
            WEX::Common::String m_file1;
            WEX::Common::String m_file2;
            std::map<std::wstring, std::wstring> m_variables;
        };
        
        // Image data comparison with diff output.

        class ImageComparer : public VerificationComparer
        {
        public:
            void SetDiffFilePath(
                _In_ const wrl::Wrappers::HStringReference& diffFilePath);

            void SetErrorFilePath(
                _In_ const wrl::Wrappers::HStringReference& errorFilePath);

            // Per-channel pixel tolerance. Pixels that differ by at most this
            // amount in every channel (R, G, B, A) are treated as matching.
            // Default is 0 (exact match).
            void SetTolerance(int tolerance) { m_tolerance = tolerance; }

            bool CompareFiles(
                _In_ const wrl::Wrappers::HStringReference& s1,
                _In_ const wrl::Wrappers::HStringReference& s2) override;

            void OutputAdditionalResults(
                bool isXmlFile) override;

        private:
            wrl::ComPtr<IWICBitmapSource> LoadWicBitmapSource(const wrl::Wrappers::HStringReference& file);
            
            wrl::ComPtr<IWICImagingFactory> m_wicImagingFactory;

            WEX::Common::String m_diffFilePath;
            WEX::Common::String m_errorFilePath;

            ImageBuffer m_diffBuffer;
            ImageBuffer m_errorBuffer;

            int m_tolerance = 0;
        };

        // Validation of XML tree dumps.

        class VisualTreeXMLComparer : public VerificationComparer
        {
        public:
            VisualTreeXMLComparer();

            void SetRulesFile(
                _In_ const wrl::Wrappers::HStringReference& rulesFilename);

            void SetRulesInline(
                _In_ const wrl::Wrappers::HStringReference& rulesXml);

            bool CompareFiles(
                _In_ const wrl::Wrappers::HStringReference& s1,
                _In_ const wrl::Wrappers::HStringReference& s2) override;

            void OutputAdditionalResults(
                bool isXmlFile) override;

        private:
            void RedirectStreamOutputToLog(
                _In_ HANDLE handle);

            void CreateOutputStreamPipe(
                _Out_ PHANDLE readHandle,
                _Out_ PHANDLE writeHandle,
                _In_ SECURITY_ATTRIBUTES* securityAttributes);

            WEX::Common::String CreateCommandLine(
                _In_ const wrl::Wrappers::HStringReference& s1,
                _In_ const wrl::Wrappers::HStringReference& s2);

            bool m_rulesAreInline;
            WEX::Common::String m_rules;
        };
    }
}
