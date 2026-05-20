// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <functional>

namespace Private { namespace Tests { namespace Framework { 
    namespace MarkupExtensions
    {
        // This custom markup extension has an internal vector of song lyrics.
        // Setting the 'LineNumber' property will result in the ProvideValue()
        // override returning the appropriate line from the lyrics vector.
        // Specific lines can also be retrieved via LookupLineNumber(), which
        // is useful for verification of ProvideValue() behavior.
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class PewPewExtension sealed
            : public Microsoft::UI::Xaml::Markup::MarkupExtension
        {
        public:
            PewPewExtension()
            {
                if (!s_initialized)
                {
                    s_lines.push_back(ref new Platform::String(L"So then it's up with the Blue and Gold"));
                    s_lines.push_back(ref new Platform::String(L"Down with the Red; red, red, red"));
                    s_lines.push_back(ref new Platform::String(L"California's out for a victory"));
                    s_lines.push_back(ref new Platform::String(L"We'll drop our battle axe on Stanford's head; chop"));
                    s_lines.push_back(ref new Platform::String(L"When we meet her, our team will surely beat her"));
                    s_lines.push_back(ref new Platform::String(L"Down on the Stanford farm, there'll be no sound"));
                    s_lines.push_back(ref new Platform::String(L"When our Oski rips through the air"));
                    s_lines.push_back(ref new Platform::String(L"Like our friend Mister Jonah, Stanford's team will be found"));
                    s_lines.push_back(ref new Platform::String(L"In the tummy of the Golden Bear"));

                    s_initialized = true;
                }
            }

            property int LineNumber
            {
                int get() { return m_lineNumber; }
                void set(int value) { m_lineNumber = value; }
            }

            static Platform::String^ LookupLineNumber(int lineNumber)
            {
                return s_lines[lineNumber];
            }

        protected:
            virtual Object^ ProvideValue() override;

        private:
            int m_lineNumber = 0;
            static Microsoft::UI::Xaml::DependencyProperty^ s_lineNumberProperty;
            static std::vector<Platform::String^> s_lines;
            static bool s_initialized;
        };

        // This custom markup extension adds the two operand properties.
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class AdditionExtension sealed
            : public Microsoft::UI::Xaml::Markup::MarkupExtension
        {
        public:
            AdditionExtension() {}

            property int Operand1
            {
                int get() { return m_operand1; }
                void set(int value) { m_operand1 = value; }
            }

            property int Operand2
            {
                int get() { return m_operand2; }
                void set(int value) { m_operand2 = value; }
            }

        protected:
            virtual Object^ ProvideValue() override;

        private:
            int m_operand1 = 0;
            int m_operand2 = 0;
        };

        // This custom markup extension returns a Binding with the specified
        // Source and Path
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class BindingFactoryExtension sealed
            : public Microsoft::UI::Xaml::Markup::MarkupExtension
        {
        public:
            BindingFactoryExtension() {}

            property Platform::String^ Path
            {
                Platform::String^ get() { return m_path; }
                void set(Platform::String^ value) { m_path = value; }
            }

            property Object^ Source
            {
                Object^ get() { return m_source; }
                void set(Object^ value) { m_source = value; }
            }

        protected:
            virtual Object^ ProvideValue() override;

        private:
            Platform::String^ m_path;
            Object^ m_source;
        };

        // This custom markup extension invokes the previously-set static callback function
        // when ProvideValue is called. ProvideValue itself simply returns nullptr
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class InvokeStaticCallbackExtension sealed
            : public Microsoft::UI::Xaml::Markup::MarkupExtension
        {
        public:
            InvokeStaticCallbackExtension() {}

        internal:
            static void SetStaticCallback(std::function<void(Microsoft::UI::Xaml::IXamlServiceProvider^)> callback);

        protected:
            virtual Object^ ProvideValue(Microsoft::UI::Xaml::IXamlServiceProvider^ serviceProvider) override;
        
        private:
            static std::function<void(Microsoft::UI::Xaml::IXamlServiceProvider^)> m_callback;
        };
    }
} } }