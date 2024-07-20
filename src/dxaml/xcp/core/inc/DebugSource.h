// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <map>
#include <stack>

// Struct to hold the file name and size information of XAML components
// This is used to provide information to debugging tools such as XamlSnoop
struct SourceFileInformation
{
    unsigned int m_uiLineNumber;
    unsigned int m_uiLinePosition;
    xstring_ptr m_spStrFilePath;

    SourceFileInformation()
        : m_uiLineNumber(0)
        , m_uiLinePosition(0)
    {}

    SourceFileInformation(
        _In_ unsigned int uiLineNumber,
        _In_ unsigned int uiLinePosition,
        _In_ const xstring_ptr& spStrFilePath)
        : m_uiLineNumber(uiLineNumber)
        , m_uiLinePosition(uiLinePosition)
        , m_spStrFilePath(spStrFilePath)
    {}
};

class DOSourceFileInformation
{
public:
    DOSourceFileInformation();

    ~DOSourceFileInformation();

    DOSourceFileInformation(
        _In_ unsigned int uiLineNumber,
        _In_ unsigned int uiLinePosition,
        _In_ const xstring_ptr& spStrFilePath);

    void AddPropertySource(
        _In_ KnownPropertyIndex dPropertyIndex,
        _In_ unsigned int uiLineNumber,
        _In_ unsigned int uiLinePosition,
        _In_ const xstring_ptr& spStrFilePath);

    HRESULT GetPropertySource(
        _In_ KnownPropertyIndex dPropertyIndex,
        _Out_ SourceFileInformation* pPropertySource);

    SourceFileInformation m_objectSourceInformation;
    std::map<KnownPropertyIndex, SourceFileInformation> m_propertySourceInformation;
    unsigned __int64 m_creationTime;
};

// This class is used for looking up source information for XAML dependency objects.
class SourceFileInformationLookup
{
public:
    SourceFileInformationLookup();
    ~SourceFileInformationLookup();

    void InsertObjectSourceInformation(
        _In_ CDependencyObject* pDependencyObject,
        _In_ const xstring_ptr& spStrFilePath,
        _In_ unsigned int uiLineNumber,
        _In_ unsigned int uiLinePosition);

    void InsertPropertySourceInformation(
        _In_ CDependencyObject* pDependencyObject,
        _In_ KnownPropertyIndex uiIndex,
        _In_ const xstring_ptr& spStrFilePath,
        _In_ unsigned int uiLineNumber,
        _In_ unsigned int uiLinePosition);

    HRESULT GetObjectSourceInformation(
        _In_ CDependencyObject* pDependencyObject,
        _Out_ SourceFileInformation* pSource);

    HRESULT GetPropertySourceInformation(
        _In_ CDependencyObject* pDependencyObject,
        _In_ KnownPropertyIndex uiIndex,
        _Out_ SourceFileInformation* pSource);

    HRESULT GetObjectCreationTime(
        _In_ CDependencyObject* pDependencyObject,
        _Out_ unsigned __int64* ptTime);

    HRESULT LogObjectCreationStart();

    HRESULT LogObjectCreationEnd(
        _In_ CDependencyObject* pDependencyObject);

private:
    static XUINT64 GetCPUTicks();

    std::map<CDependencyObject*, std::shared_ptr<DOSourceFileInformation>> m_objectSourceFileInformationMap;
    std::map<xstring_ptr, xstring_ptr> m_fileToIdMap;
    std::stack<unsigned __int64> m_startTimesStack;
};

