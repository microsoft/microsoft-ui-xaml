// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class PropertyPathListener;
    
    class PropertyPathStepDescriptor
    {
    public:

        virtual ~PropertyPathStepDescriptor()
        { }
        
    public:

        virtual _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener *pListener, 
            bool fListenToChanges, 
            _Outptr_ PropertyPathStep **ppStep) = 0;
    };

    class SourceAccessPathStepDescriptor:
        public PropertyPathStepDescriptor
    {
    public:

        _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener *pListener, 
            bool fListenToChanges, 
            _Outptr_ PropertyPathStep **ppStep) override;
    };

    class PropertyAccessPathStepDescriptor:
        public PropertyPathStepDescriptor
    {
    public:

        PropertyAccessPathStepDescriptor(_In_z_ WCHAR *szName);
        ~PropertyAccessPathStepDescriptor() override;

    public: 

        _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener *pListener, 
            bool fListenToChanges, 
            _Outptr_ PropertyPathStep **ppStep) override;

    private:

        WCHAR *m_szName;
    };

    class IntIndexerPathStepDescriptor:
        public PropertyPathStepDescriptor
    {
    public:

        IntIndexerPathStepDescriptor(XUINT32 nIndex);
        
    public:

        _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener *pListener, 
            bool fListenToChanges, 
            _Outptr_ PropertyPathStep **ppStep) override;

    private:

        XUINT32 m_nIndex;
    };

    class StringIndexerPathStepDescriptor:
        public PropertyPathStepDescriptor
    {
    public:

        StringIndexerPathStepDescriptor(_In_z_ WCHAR *szIndex);
        ~StringIndexerPathStepDescriptor() override;

    public:

        _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener *pListener, 
            bool fListenToChanges, 
            _Outptr_ PropertyPathStep **ppStep) override;

    private:

        WCHAR *m_szIndex;
    };


    class DependencyPropertyPathStepDescriptor:
        public PropertyPathStepDescriptor
    {
    public:

        DependencyPropertyPathStepDescriptor(_In_ const CDependencyProperty *pDP);
        ~DependencyPropertyPathStepDescriptor() override;

    public:

        _Check_return_ HRESULT CreateStep(
            _In_ PropertyPathListener *pListener, 
            bool fListenToChanges, 
            _Outptr_ PropertyPathStep **ppStep) override;

    private:

        const CDependencyProperty *m_pDP;
        
    };
}
