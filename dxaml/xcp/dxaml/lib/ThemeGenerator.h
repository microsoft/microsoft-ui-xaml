// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides an API for ThemeGenerator implementation.
//
// To slow down transitions in debug builds, set the DWORD HKLM\Software\Microsoft\DirectUI\ThemeAnimationSlowDownFactor.
// See m_dbgSlowDownFactor and ThemeGeneratorHelper::Initialize.

#pragma once

namespace DirectUI
{
    class DoubleAnimationUsingKeyFrames;
    class PointerAnimationUsingKeyFrames;
    class StaggerFunctionBase;

    struct TimingFunctionDescription
    {
        wf::Point cp1;
        wf::Point cp2;
        wf::Point cp3;
        wf::Point cp4;

        TimingFunctionDescription()
        {
            cp1.X = 0.0f;
            cp1.Y = 0.0f;
            cp2.X = 0.0f;
            cp2.Y = 0.0f;
            cp3.X = 1.0f;
            cp3.Y = 1.0f;
            cp4.X = 1.0f;
            cp4.Y = 1.0f;
        }

        BOOLEAN IsLinear()
        {
            return cp1.X == 0.0f && cp1.Y == 0.0f && cp2.X == 0.0f && cp2.Y == 0.0f && cp3.X == 1.0f && cp3.Y  == 1.0f && cp4.X == 1.0f &&  cp4.Y == 1.0f;
        }
    };

    //------------------------------------------------------------------------
    // helper class. Responsibility: dealing with Storyboard specifics.
    //
    // Keyframes have to be grouped into keyframecollections
    // This class takes care of carrying that state and is a 1-1 mapping to one
    // call to one of the generate storyboards methods.
    //------------------------------------------------------------------------
    class ThemeGeneratorHelper
    {
        // we are going to create animations. All these keyframes are going to have to
        // be put into doubleanimationusingkeyframes.
        // this is a mapping between the propertyname and the keyframecollection.
    public:
        ThemeGeneratorHelper(
            _In_ wf::Point startOffset,
            _In_ wf::Point destinationOffset,
            _In_ HSTRING targetName,
            _In_opt_ xaml::IDependencyObject* pTarget,
            _In_ BOOLEAN onlyGenerateSteadyState,
            _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection) :
            m_originValuesSet(FALSE),
            m_clipValuesSet(FALSE),
            m_isHitTestVisibleValueSet(FALSE),
            m_pTimelines(timelineCollection),
            m_startOffset(startOffset),
            m_destinationOffset(destinationOffset),
            m_targetName(targetName),
            m_onlyGenerateSteadyState(onlyGenerateSteadyState),
            m_begintime(0),
            m_additionalTime(0),
            m_target(NULL),
#if DBG
            m_dbgSlowDownFactor(1),
#endif
            m_initialOpacity(0),
            m_overrideInitialOpacity(FALSE)
        {
            ReplaceInterface(m_target, pTarget);
        };

        ~ThemeGeneratorHelper();

    private:
        wfc::IVector<xaml_animation::Timeline*>* m_pTimelines;

        INT64 m_begintime;   // the begintime set on the timeline
        INT64 m_additionalTime; // additional time, that might be used

        BOOLEAN m_originValuesSet;
        BOOLEAN m_clipValuesSet;
        BOOLEAN m_isHitTestVisibleValueSet;
        BOOLEAN m_onlyGenerateSteadyState;

        wf::Point m_startOffset;
        wf::Point m_destinationOffset;

        IActivationFactory* m_pActivationFactory{};
        xaml_animation::IStoryboardStatics* m_pStoryboardStatics;

        wrl_wrappers::HString m_strTranslateXPropertyName;
        wrl_wrappers::HString m_strTranslateYPropertyName;
        wrl_wrappers::HString m_strOpacityPropertyName;
        wrl_wrappers::HString m_strCenterYPropertyName;
        wrl_wrappers::HString m_strScaleXPropertyName;
        wrl_wrappers::HString m_strScaleYPropertyName;
        wrl_wrappers::HString m_strClipScaleXPropertyName;
        wrl_wrappers::HString m_strClipScaleYPropertyName;
        wrl_wrappers::HString m_strClipTranslateXPropertyName;
        wrl_wrappers::HString m_strClipTranslateYPropertyName;

        HSTRING m_targetName;
        xaml::IDependencyObject* m_target;
        wrl_wrappers::HString m_strOverrideTranslateXPropertyName;
        wrl_wrappers::HString m_strOverrideTranslateYPropertyName;
        std::map<HSTRING, DoubleAnimationUsingKeyFrames*> m_doubleAnimationsMap;
        std::map<HSTRING, PointerAnimationUsingKeyFrames*> m_pointerAnimationsMap;

#if DBG
        INT64 m_dbgSlowDownFactor;
#endif

        DOUBLE m_initialOpacity;
        BOOLEAN m_overrideInitialOpacity;


    public:
        _Check_return_ HRESULT Initialize();


    public:
        _Check_return_ wf::Point GetStartOffset() { return m_startOffset; }
        _Check_return_ wf::Point GetDestinationOffset() { return m_destinationOffset; }

        HSTRING GetTranslateXPropertyName() const { return m_strOverrideTranslateXPropertyName.Get() ? m_strOverrideTranslateXPropertyName.Get() : m_strTranslateXPropertyName.Get(); }
        HSTRING GetTranslateYPropertyName() const { return m_strOverrideTranslateYPropertyName.Get() ? m_strOverrideTranslateYPropertyName.Get() : m_strTranslateYPropertyName.Get(); }
        HSTRING GetOpacityPropertyName() const { return m_strOpacityPropertyName.Get(); }
        HSTRING GetCenterYPropertyName() const { return m_strCenterYPropertyName.Get(); }
        HSTRING GetScaleXPropertyName() const { return m_strScaleXPropertyName.Get(); }
        HSTRING GetScaleYPropertyName() const { return m_strScaleYPropertyName.Get(); }
        HSTRING GetClipScaleXPropertyName() const { return m_strClipScaleXPropertyName.Get(); }
        HSTRING GetClipScaleYPropertyName() const { return m_strClipScaleYPropertyName.Get(); }
        HSTRING GetClipTranslateXPropertyName() const { return m_strClipTranslateXPropertyName.Get(); }
        HSTRING GetClipTranslateYPropertyName() const { return m_strClipTranslateYPropertyName.Get(); }

        _Check_return_ HRESULT RegisterKeyFrame(
            _In_ HSTRING targetPropertyName,
            _In_ DOUBLE value,
            _In_ INT64 begintime,
            _In_ INT64 duration,
            _In_opt_ TimingFunctionDescription* pEasing);

        _Check_return_ HRESULT RegisterPointerKeyFrame(
            _In_ HSTRING targetPropertyName,
            _In_ PointerDirection sourceDirection,
            _In_ DOUBLE pointerValue,
            _In_ DOUBLE value,
            _In_ INT64 begintime);

        _Check_return_ HRESULT Set2DTransformOriginValues(_In_ wf::Point originPoint);

        _Check_return_ HRESULT SetClipOriginValues(_In_ wf::Point originPoint);

        _Check_return_ HRESULT PreventHitTestingWhileAnimating(_In_ INT64 durationInMilliseconds);

        _Check_return_ HRESULT SetOverrideTranslateXPropertyName(_In_reads_(nLength) const WCHAR *pszValue, _In_ XUINT32 nLength)
        {
            RRETURN(m_strOverrideTranslateXPropertyName.Set(pszValue, nLength));
        }

        _Check_return_ HRESULT SetOverrideTranslateYPropertyName(_In_reads_(nLength) const XCHAR *pszValue, _In_ XUINT32 nLength)
        {
            RRETURN(m_strOverrideTranslateYPropertyName.Set(pszValue, nLength));
        }

        void SetOverrideInitialOpacity(DOUBLE initialOpacity) { m_initialOpacity = initialOpacity; m_overrideInitialOpacity = TRUE; }
        BOOLEAN GetInitialOpacity(DOUBLE& fallbackValue) { fallbackValue = m_initialOpacity; return m_overrideInitialOpacity; }

        void SetAdditionalTime(_In_ INT64 time) { m_additionalTime = time; }
        _Check_return_ INT64 GetAdditionalTime() { return m_additionalTime; }

        _Check_return_ HRESULT SetOverrideTarget(_In_ xaml::IDependencyObject* pTarget);

    private:
        _Check_return_ HRESULT SetOriginForProperty(_In_ wf::Point originPoint, _In_ HSTRING propertyName, _In_ BOOLEAN& valueSet);
    };

    class ThemeGenerator
    {
        public:
            _Check_return_ static HRESULT AddTimelinesForThemeAnimation(
                _In_ INT storyboardID,
                _In_ INT targetID,
                _In_ ThemeGeneratorHelper* pHelper);

            _Check_return_ static HRESULT AddTimelinesForThemeAnimation(
                _In_ INT storyboardID,
                _In_ INT targetID,
                _In_ HSTRING targetName,      // can be a zero sized string
                _In_opt_ xaml::IDependencyObject* pTarget,   // can be null
                _In_ BOOLEAN onlyGenerateSteadyState,
                _In_ wf::Point startOffset,
                _In_ wf::Point destinationOffset,
                _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection);

            _Check_return_ static HRESULT AddTimelinesForThemeAnimation(
                _In_ INT storyboardID,
                _In_ INT targetID,
                _In_ BOOLEAN onlyGenerateSteadyState,
                _In_ wf::Point startOffset,
                _In_ wf::Point destinationOffset,
                _In_ INT64 additionalTime,
                _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection);

            _Check_return_ static HRESULT GetStaggerFunction(
                _In_ INT storyboardID,
                _In_ INT targetID,
                _Outptr_ StaggerFunctionBase** ppInstance);
    };
}

