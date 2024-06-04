// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include "DurationVO.h"

class CCoreServices;
class CStoryboard;
class CControl;
class CFrameworkElement;
class CVisualState;
class CVisualStateGroup;
class CDependencyObject;
class CEventArgs;

class CVisualTransition final
    : public CDependencyObject
{
private:
    CVisualTransition(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

protected:
    ~CVisualTransition() override;

public:
    DECLARE_CREATE(CVisualTransition);

    _Check_return_ HRESULT InitInstance() override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVisualTransition>::Index;
    }

#pragma region Legacy VSM Code
    bool GetIsDefault();

    bool IsZeroDuration();

    // visualtransitions should only be registered by the parser
    _Check_return_ HRESULT RegisterName( _In_ CDependencyObject *pNamescopeOwner, _In_ XUINT32 bTemplateNamescope) override;
#pragma endregion

    xref_ptr<DurationVO::Wrapper>   m_duration;
    CStoryboard*                    m_pStoryboard       = nullptr;
    CDependencyObject*              m_pEasingFunction   = nullptr;
    xstring_ptr                     m_strTo;
    xstring_ptr                     m_strFrom;
};