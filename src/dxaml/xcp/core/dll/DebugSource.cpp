// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

DOSourceFileInformation::DOSourceFileInformation()
: m_creationTime(0)
{}

DOSourceFileInformation::~DOSourceFileInformation()
{}

DOSourceFileInformation::DOSourceFileInformation(
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition,
    _In_ const xstring_ptr& spStrFilePath)
    : m_creationTime(0)
    , m_objectSourceInformation(uiLineNumber, uiLinePosition, spStrFilePath)
{}

void DOSourceFileInformation::AddPropertySource(
    _In_ KnownPropertyIndex dPropertyIndex,
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition,
    _In_ const xstring_ptr& spStrFilePath)
{
    m_propertySourceInformation.emplace(
        dPropertyIndex,
        SourceFileInformation(uiLineNumber, uiLinePosition, spStrFilePath));
}

HRESULT DOSourceFileInformation::GetPropertySource(
    _In_ KnownPropertyIndex dPropertyIndex,
    _Out_ SourceFileInformation* pPropertySource)
{
    HRESULT hr = S_OK;

    *pPropertySource = m_propertySourceInformation[dPropertyIndex];

    RRETURN(hr);//RRETURN_REMOVAL
}

SourceFileInformationLookup::SourceFileInformationLookup()
{}

SourceFileInformationLookup::~SourceFileInformationLookup()
{}

void SourceFileInformationLookup::InsertObjectSourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _In_ const xstring_ptr& spStrFilePath,
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition)
{
    if (m_objectSourceFileInformationMap.find(pDependencyObject) == m_objectSourceFileInformationMap.end())
    {
        xstring_ptr spStrFilePtr;

        if (!spStrFilePath.IsNullOrEmpty())
        {
            m_fileToIdMap.emplace(spStrFilePath, spStrFilePath);
            spStrFilePtr = spStrFilePath;
        }

        m_objectSourceFileInformationMap.emplace(pDependencyObject, 
            std::make_shared<DOSourceFileInformation>(uiLineNumber, uiLinePosition, spStrFilePtr));
    }
}

void SourceFileInformationLookup::InsertPropertySourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _In_ KnownPropertyIndex uiIndex,
    _In_ const xstring_ptr& spStrFilePath,
    _In_ unsigned int uiLineNumber,
    _In_ unsigned int uiLinePosition)
{
    std::shared_ptr<DOSourceFileInformation> spDependencySource;

    spDependencySource = m_objectSourceFileInformationMap[pDependencyObject];

    if (spDependencySource)
    {
        xstring_ptr spStrFilePtr;

        if (!spStrFilePath.IsNullOrEmpty())
        {
            m_fileToIdMap.emplace(spStrFilePath, spStrFilePath);
            spStrFilePtr = spStrFilePath;
        }

        spDependencySource->AddPropertySource(uiIndex, uiLineNumber, uiLinePosition, spStrFilePtr);
    }
}

HRESULT SourceFileInformationLookup::GetObjectSourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _Out_ SourceFileInformation* pSource)
{
    HRESULT hr = S_OK;

    std::shared_ptr<DOSourceFileInformation> spObjectSource;

    spObjectSource = m_objectSourceFileInformationMap[pDependencyObject];

    if (spObjectSource)
    {
        *pSource = spObjectSource->m_objectSourceInformation;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

HRESULT SourceFileInformationLookup::GetPropertySourceInformation(
    _In_ CDependencyObject* pDependencyObject,
    _In_ KnownPropertyIndex uiIndex,
    _Out_ SourceFileInformation* pSource)
{
    std::shared_ptr<DOSourceFileInformation> spObjectSource;

    spObjectSource = m_objectSourceFileInformationMap[pDependencyObject];

    if (spObjectSource)
    {
        IFC_RETURN(spObjectSource->GetPropertySource(uiIndex, pSource));
    }

    return S_OK;
}

unsigned __int64 SourceFileInformationLookup::GetCPUTicks()
{
    LARGE_INTEGER liQPCElapsed;

    QueryPerformanceCounter(&liQPCElapsed);

    return static_cast<unsigned __int64>(liQPCElapsed.QuadPart);
}

HRESULT SourceFileInformationLookup::LogObjectCreationStart()
{
    HRESULT hr = S_OK;

    m_startTimesStack.push(GetCPUTicks());

    RRETURN(hr);//RRETURN_REMOVAL
}

HRESULT SourceFileInformationLookup::LogObjectCreationEnd(
    _In_ CDependencyObject* pDependencyObject)
{
    HRESULT hr = S_OK;

    unsigned __int64 stopTime = GetCPUTicks();
    unsigned __int64 startTime = 0;
    std::shared_ptr<DOSourceFileInformation> spObjectSource;

    spObjectSource = m_objectSourceFileInformationMap[pDependencyObject];

    m_startTimesStack.push(startTime);
    m_startTimesStack.pop();

    if (spObjectSource && spObjectSource->m_creationTime == 0)
    {
        spObjectSource->m_creationTime = stopTime - startTime;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

HRESULT SourceFileInformationLookup::GetObjectCreationTime(
    _In_ CDependencyObject* pDependencyObject,
    _Out_ unsigned __int64* ptTime)
{
    HRESULT hr = S_OK;
    std::shared_ptr<DOSourceFileInformation> spObjectSource;

    *ptTime = 0;

    spObjectSource = m_objectSourceFileInformationMap[pDependencyObject];

    if (spObjectSource)
    {
        *ptTime = spObjectSource->m_creationTime;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

