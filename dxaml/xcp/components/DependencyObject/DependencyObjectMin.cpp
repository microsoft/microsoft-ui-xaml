// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <MetadataAPI.h>
#include <TypeTableStructs.h>
#include <CDependencyObject.h>
#include <corep.h>

const CClassInfo* CDependencyObject::GetClassInformation() const
{
    return DirectUI::MetadataAPI::GetClassInfoByIndex(GetTypeIndex());
}

bool CDependencyObject::OfTypeByIndex(_In_ KnownTypeIndex nIndex) const
{
    return DirectUI::MetadataAPI::IsAssignableFrom(nIndex, GetTypeIndex());
}

const CDependencyProperty* CDependencyObject::GetContentProperty()
{
    return GetClassInformation()->GetContentProperty();
}

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
bool CDependencyObject::IsStrictType() const
{
    return GetClassInformation()->IsStrict();
}
#endif

void
CDependencyObject::AddRef()
{
#ifdef DBG
    if (GetContext() && GetContext()->DbgIsThreadingAssertEnabled())
    {
        ASSERT(GetCurrentThreadId() == GetContext()->GetThreadID(), L"AddRef on non-UI thread");
    }
#endif
    const auto cRef = m_requiresThreadSafeAddRefRelease ? m_ref_count.ThreadSafeAddRef() : m_ref_count.AddRef();

    if (!cRef)
    {
        // we have hit an overflow...exit the process
        ASSERT(!"AddRef Overflow");
        XAML_FAIL_FAST();
    }

    AddRefImpl(cRef);
}

void
CDependencyObject::Release()
{
#ifdef DBG
    if (GetContext() && GetContext()->DbgIsThreadingAssertEnabled())
    {
        ASSERT(GetCurrentThreadId() == GetContext()->GetThreadID(), L"Release on non-UI thread");
    }
#endif
    if (m_requiresReleaseOverride)
    {
        ReleaseOverride();
    }

    const auto cRef = m_requiresThreadSafeAddRefRelease ? m_ref_count.ThreadSafeRelease() : m_ref_count.Release();
    ReleaseImpl(cRef);
}

