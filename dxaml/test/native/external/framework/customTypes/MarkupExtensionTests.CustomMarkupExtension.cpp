// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MarkupExtensionTests.CustomMarkupExtension.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Data;

namespace Tests { namespace Native { namespace External { namespace Framework {
    namespace MarkupExtensions
    {
        bool PewPewExtension::s_initialized = false;
        std::vector<Platform::String^> PewPewExtension::s_lines;
        std::function<void(Microsoft::UI::Xaml::IXamlServiceProvider^)> InvokeStaticCallbackExtension::m_callback;

        Platform::Object^ PewPewExtension::ProvideValue()
        {
            return s_lines[LineNumber];
        }

        Platform::Object^ AdditionExtension::ProvideValue()
        {
            return m_operand1 + m_operand2;
        }

        Platform::Object^ BindingFactoryExtension::ProvideValue()
        {
            auto binding = ref new Binding();
            binding->Source = m_source;
            binding->Path = ref new PropertyPath(m_path);

            return binding;
        }

        Platform::Object^ InvokeStaticCallbackExtension::ProvideValue(Microsoft::UI::Xaml::IXamlServiceProvider^ serviceProvider)
        {
            m_callback(serviceProvider);
            return nullptr;
        }

        /* static */ void InvokeStaticCallbackExtension::SetStaticCallback(std::function<void(Microsoft::UI::Xaml::IXamlServiceProvider^)> callback)
        {
            m_callback = callback;
        }
    }
} } } }