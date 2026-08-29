// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

using namespace test_infra;
using namespace ::Windows::Foundation;

template<class T>
ref class CustomMetadataRegistrar sealed : ICustomMetadataRegistrar
{
public:
    CustomMetadataRegistrar()
        : m_disposed(false)
    {
    }

    virtual void RegisterMetadata()
    {
        T::RegisterDependencyProperties();
    }

    virtual void Dispose()
    {
        if (!m_disposed)
        {
            m_disposed = true;
            T::ClearDependencyProperties();
        }
    }

    virtual ~CustomMetadataRegistrar()
    {
        if (!m_disposed)
        {
            Dispose();
        }
    }

private:
    bool m_disposed;

};



} } } } }
