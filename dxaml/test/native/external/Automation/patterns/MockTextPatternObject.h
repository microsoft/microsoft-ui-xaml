// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Patterns {

delegate bool ITextRangeProviderCompare(Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ range);

ref class MockUpProviderControlRange sealed : public Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider
{
public:
    MockUpProviderControlRange()
    {
        initialize();
    };

    virtual ~MockUpProviderControlRange()
    {
        initialize();
    };

public:
    void initialize(){
        for (UINT u = 0; u < 40; ++u)
        {
            _rgAttributes[u] = nullptr;
        }
    }

    void setAttribute(TEXTATTRIBUTEID attributeId, Platform::Object^ prop){
        switch (attributeId)
        {
        case UIA_AnimationStyleAttributeId:
        case UIA_BackgroundColorAttributeId:
        case UIA_BulletStyleAttributeId:
        case UIA_CapStyleAttributeId:
        case UIA_CultureAttributeId:
        case UIA_FontNameAttributeId:
        case UIA_FontSizeAttributeId:
        case UIA_FontWeightAttributeId:
        case UIA_ForegroundColorAttributeId:
        case UIA_HorizontalTextAlignmentAttributeId:
        case UIA_IndentationFirstLineAttributeId:
        case UIA_IndentationLeadingAttributeId:
        case UIA_IndentationTrailingAttributeId:
        case UIA_IsHiddenAttributeId:
        case UIA_IsItalicAttributeId:
        case UIA_IsReadOnlyAttributeId:
        case UIA_IsSubscriptAttributeId:
        case UIA_IsSuperscriptAttributeId:
        case UIA_MarginBottomAttributeId:
        case UIA_MarginLeadingAttributeId:
        case UIA_MarginTopAttributeId:
        case UIA_MarginTrailingAttributeId:
        case UIA_OutlineStylesAttributeId:
        case UIA_OverlineColorAttributeId:
        case UIA_OverlineStyleAttributeId:
        case UIA_StrikethroughColorAttributeId:
        case UIA_StrikethroughStyleAttributeId:
        case UIA_TabsAttributeId:
        case UIA_TextFlowDirectionsAttributeId:
        case UIA_UnderlineColorAttributeId:
        case UIA_UnderlineStyleAttributeId:
        case UIA_AnnotationTypesAttributeId:
        case UIA_AnnotationObjectsAttributeId:
        case UIA_StyleNameAttributeId:
        case UIA_StyleIdAttributeId:
        case UIA_LinkAttributeId:
        case UIA_IsActiveAttributeId:
        case UIA_SelectionActiveEndAttributeId:
        case UIA_CaretPositionAttributeId:
        case UIA_CaretBidiModeAttributeId:
            _rgAttributes[attributeId - UIA_AnimationStyleAttributeId] = prop;
            break;
        default:
            break;
        }
    }

    // ITextRangeProvider

    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ Clone(){ return nullptr; };

    virtual bool Compare(Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ range)
    { 
        // We allow the body of this function to be replaced via CompareImpl property.
        if (CompareImpl == nullptr)
        {
            return range == this;
        }
        else
        {
            return CompareImpl(range);
        }
    };
    property ITextRangeProviderCompare^ CompareImpl;

    virtual int CompareEndpoints(Microsoft::UI::Xaml::Automation::Text::TextPatternRangeEndpoint, Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^, Microsoft::UI::Xaml::Automation::Text::TextPatternRangeEndpoint){ return E_NOTIMPL; };
    virtual void ExpandToEnclosingUnit(Microsoft::UI::Xaml::Automation::Text::TextUnit){};
    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ FindAttribute(TEXTATTRIBUTEID attributeId, Platform::Object^ objExpected, bool /*direction*/)
    {
        Platform::Object^ objCurrent = GetAttributeValue(attributeId);
        if (objCurrent != nullptr && objExpected != nullptr)
        {
            if (objCurrent == nullptr && objExpected == nullptr)
            {
                return this;
            }
            else if (attributeId == UIA_TabsAttributeId)
            {
                wfc::IVectorView<double>^ vecviewCurrent = static_cast<wfc::IVectorView<double>^>(objCurrent);
                wfc::IVectorView<double>^ vecviewExpected = static_cast<wfc::IVectorView<double>^>(objExpected);
                if (vecviewCurrent->Size == vecviewExpected->Size)
                {
                    for (UINT i = 0; i < vecviewCurrent->Size; ++i)
                    {
                        if (vecviewCurrent->GetAt(i) != vecviewExpected->GetAt(i))
                        {
                            return nullptr;
                        }
                    }
                    return this;
                }
            }
            else if (attributeId == UIA_AnnotationTypesAttributeId)
            {
                wfc::IVectorView<int>^ vecviewCurrent = static_cast<wfc::IVectorView<int>^>(objCurrent);
                wfc::IVectorView<int>^ vecviewExpected = static_cast<wfc::IVectorView<int>^>(objExpected);
                if (vecviewCurrent->Size == vecviewExpected->Size)
                {
                    for (UINT i = 0; i < vecviewCurrent->Size; ++i)
                    {
                        if (vecviewCurrent->GetAt(i) != vecviewExpected->GetAt(i))
                        {
                            return nullptr;
                        }
                    }
                    return this;
                }
            }
            else
            {
                Platform::Type ^t1 = objCurrent->GetType();
                Platform::Type ^t2 = objExpected->GetType();
                if (Platform::String::CompareOrdinal(t1->ToString(), t1->ToString()) == 0)
                {
                    if (Platform::String::CompareOrdinal(objCurrent->ToString(), objExpected->ToString()) == 0)
                    {
                        return this;
                    }
                }
            }
        }
        return nullptr;
    };

    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ FindText(Platform::String^, bool, bool){ return nullptr; };
    virtual Platform::Object^ GetAttributeValue(TEXTATTRIBUTEID attributeId){
        Platform::Object^ _retValue = nullptr;
        switch (attributeId){
        case UIA_AnimationStyleAttributeId:
        case UIA_BackgroundColorAttributeId:
        case UIA_BulletStyleAttributeId:
        case UIA_CapStyleAttributeId:
        case UIA_CultureAttributeId:
        case UIA_FontNameAttributeId:
        case UIA_FontSizeAttributeId:
        case UIA_FontWeightAttributeId:
        case UIA_ForegroundColorAttributeId:
        case UIA_HorizontalTextAlignmentAttributeId:
        case UIA_IndentationFirstLineAttributeId:
        case UIA_IndentationLeadingAttributeId:
        case UIA_IndentationTrailingAttributeId:
        case UIA_IsHiddenAttributeId:
        case UIA_IsItalicAttributeId:
        case UIA_IsReadOnlyAttributeId:
        case UIA_IsSubscriptAttributeId:
        case UIA_IsSuperscriptAttributeId:
        case UIA_MarginBottomAttributeId:
        case UIA_MarginLeadingAttributeId:
        case UIA_MarginTopAttributeId:
        case UIA_MarginTrailingAttributeId:
        case UIA_OutlineStylesAttributeId:
        case UIA_OverlineColorAttributeId:
        case UIA_OverlineStyleAttributeId:
        case UIA_StrikethroughColorAttributeId:
        case UIA_StrikethroughStyleAttributeId:
        case UIA_TabsAttributeId:
        case UIA_TextFlowDirectionsAttributeId:
        case UIA_UnderlineColorAttributeId:
        case UIA_UnderlineStyleAttributeId:
        case UIA_AnnotationTypesAttributeId:
        case UIA_AnnotationObjectsAttributeId:
        case UIA_StyleNameAttributeId:
        case UIA_StyleIdAttributeId:
        case UIA_LinkAttributeId:
        case UIA_IsActiveAttributeId:
        case UIA_SelectionActiveEndAttributeId:
        case UIA_CaretPositionAttributeId:
        case UIA_CaretBidiModeAttributeId:
            _retValue = _rgAttributes[attributeId - UIA_AnimationStyleAttributeId];
            break;
        default:
            break;
        }

        return _retValue;
    };
    virtual void GetBoundingRectangles(Platform::Array<double>^ *){};
    virtual Microsoft::UI::Xaml::Automation::Provider::IRawElementProviderSimple^ GetEnclosingElement(){ return nullptr; };
    virtual Platform::String^ GetText(int){ return "MockUpProviderControlRange"; };
    virtual int Move(Microsoft::UI::Xaml::Automation::Text::TextUnit, int){ return 0; };
    virtual int MoveEndpointByUnit(Microsoft::UI::Xaml::Automation::Text::TextPatternRangeEndpoint, Microsoft::UI::Xaml::Automation::Text::TextUnit, int){ return 0; };
    virtual void MoveEndpointByRange(Microsoft::UI::Xaml::Automation::Text::TextPatternRangeEndpoint, Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^, Microsoft::UI::Xaml::Automation::Text::TextPatternRangeEndpoint){};
    virtual void Select(){};
    virtual void AddToSelection(){};
    virtual void RemoveFromSelection(){};
    virtual void ScrollIntoView(bool){};
    virtual Platform::Array<Microsoft::UI::Xaml::Automation::Provider::IRawElementProviderSimple^>^ GetChildren(){ return nullptr; };

private:
    Platform::Object^ _rgAttributes[40];
};

ref class MockUpProviderControlAutomationPeer sealed : public xaml_automation_peers::ButtonAutomationPeer,
    public Microsoft::UI::Xaml::Automation::Provider::ITextEditProvider, public Microsoft::UI::Xaml::Automation::Provider::ITextProvider2
{
public:
    MockUpProviderControlAutomationPeer(xaml_controls::Button^ owner) :xaml_automation_peers::ButtonAutomationPeer(owner), m_bMakeRangeValid(true)
    {
        _range = ref new MockUpProviderControlRange();
    };

    virtual ~MockUpProviderControlAutomationPeer()
    {
        _range = nullptr;
    };

protected:
    Platform::Object^ GetPatternCore(Microsoft::UI::Xaml::Automation::Peers::PatternInterface patternInterface) override
    {
        if (patternInterface == xaml_automation_peers::PatternInterface::TextEdit)
        {
            return this;
        }
        else if (patternInterface == xaml_automation_peers::PatternInterface::Text2)
        {
            return this;
        }
        else if (patternInterface == xaml_automation_peers::PatternInterface::Text)
        {
            return this;
        }

        return xaml_automation_peers::ButtonAutomationPeer::GetPatternCore(patternInterface);
    }

    Platform::String^ GetClassNameCore() override
    {
        return "TestMock";
    }
public:
    // ITextProvider
    virtual Platform::Array<Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^>^ GetSelection()
    {
        return nullptr;
    }
    virtual Platform::Array<Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^>^ GetVisibleRanges()
    {
        return nullptr;
    }
    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ RangeFromChild(Microsoft::UI::Xaml::Automation::Provider::IRawElementProviderSimple^ /*childElement*/)
    {
        return nullptr;
    }
    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ RangeFromPoint(::Windows::Foundation::Point /*screenLocation*/)
    {
        return nullptr;
    }
    virtual property Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ DocumentRange
    {
        Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ get()
        {
            if (m_bMakeRangeValid)
            {
                return _range;
            }
            return nullptr;
        };
    };

    virtual property Microsoft::UI::Xaml::Automation::SupportedTextSelection SupportedTextSelection
    {
        Microsoft::UI::Xaml::Automation::SupportedTextSelection get()
        {
            return Microsoft::UI::Xaml::Automation::SupportedTextSelection::Single;
        }
    };

    // ITextProvider2
    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider ^RangeFromAnnotation(Microsoft::UI::Xaml::Automation::Provider::IRawElementProviderSimple ^)
    {
        return nullptr;
    }

    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider ^GetCaretRange(bool *isActive)
    {
        *isActive = GetCaretRangeIsActiveMockValue;
        if (m_bMakeRangeValid)
        {
            return _range;
        }
        return nullptr;
    }
    property bool GetCaretRangeIsActiveMockValue;

    // ITextEditProvider
    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ GetActiveComposition()
    {
        if (m_bMakeRangeValid)
        {
            return _range;
        }
        return nullptr;
    }

    virtual Microsoft::UI::Xaml::Automation::Provider::ITextRangeProvider^ GetConversionTarget()
    {
        if (m_bMakeRangeValid)
        {
            return _range;
        }
        return nullptr;
    }

public:
    MockUpProviderControlRange^ _getRange()
    {
        return _range;
    }

    void _setRange(MockUpProviderControlRange^ range)
    {
        _range = range;
    }

    void MakeRangeValid(bool bMakeRangeValid)
    {
        m_bMakeRangeValid = bMakeRangeValid;
    }

private:
    MockUpProviderControlRange^ _range;
    bool m_bMakeRangeValid;
};

ref class MockUpProviderControl sealed : public xaml_controls::Button
{
public:
    MockUpProviderControl(){
        DefaultStyleKey = "Microsoft.UI.Xaml.Test.Automation.TextRange.MockUpProviderControl";
    }

    virtual ~MockUpProviderControl(){
    }

protected:
    virtual xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
    {
        return ref new MockUpProviderControlAutomationPeer(this);
    }
};

} } } } } }
