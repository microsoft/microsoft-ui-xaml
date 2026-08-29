// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    // We set IsHitTestVisible=False on the root element to prevent mouse interference.
    // By default, we exclude the FocusState property because depending on in
    // which order the test is executed Controls may or may not have it set,
    // leading to instability.
    // We also exclude the AutomationProperties.ControlledPeers property.
    // This is an accessibility property set on RITP machines but nowhere else on our testing environment.
    // IsPressed and IsPointerOver frequently cause tests to fail due to sometimes being False and sometimes being
    // "empty" (i.e. unset). These properties are not important for the purposes of validating control states, so we ignore them.
    static const wchar_t* DefaultUIElementTreeValidationRules =
        LR"(<?xml version='1.0' encoding='UTF-8'?>
            <Rules>
                <Rule Applicability='/VisualTreeDump/VisualRoot/Element' Inclusion='Blacklist'>
                    <Property Name='IsHitTestVisible'/>
                </Rule>
                <Rule Applicability='//Element' Inclusion='Blacklist'>
                    <Property Name='FocusState'/>
                    <Property Name='AutomationProperties.ControlledPeers'/>
                    <Property Name='IsPressed'/>
                    <Property Name='IsPointerOver'/>
                </Rule>
            </Rules>)";

    struct ValidateTreeParams
    {
        ValidateTreeParams() = delete;

        ValidateTreeParams(
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc
            )
            : ValidateTreeParams(
                L"",
                windowSizeOverride,
                scaleFactorOverride,
                setupFunc,
                nullptr,
                false,
                ref new Platform::String(DefaultUIElementTreeValidationRules),
                true
                )
        {
        }

        ValidateTreeParams(
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc,
            std::function<void()> cleanupFunc
            )
            : ValidateTreeParams(
                L"",
                windowSizeOverride,
                scaleFactorOverride,
                setupFunc,
                cleanupFunc,
                false,
                ref new Platform::String(DefaultUIElementTreeValidationRules),
                true
                )
        {
        }

        ValidateTreeParams(
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc,
            std::function<void()> cleanupFunc,
            bool disableHittestingOnRoot,
            bool ignorePopups = false
            )
            : ValidateTreeParams(
                L"",
                windowSizeOverride,
                scaleFactorOverride,
                setupFunc,
                cleanupFunc,
                false,
                ref new Platform::String(DefaultUIElementTreeValidationRules),
                disableHittestingOnRoot,
                ignorePopups
                )
        {
        }

        ValidateTreeParams(
            Platform::String^ subTestId,
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc
            )
            : ValidateTreeParams(
                subTestId,
                windowSizeOverride,
                scaleFactorOverride,
                setupFunc,
                nullptr,
                false,
                ref new Platform::String(DefaultUIElementTreeValidationRules),
                true
                )
        {
        }

        ValidateTreeParams(
            Platform::String^ subTestId,
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc,
            Platform::String^ validationRules
            )
            : ValidateTreeParams(
                subTestId,
                windowSizeOverride,
                scaleFactorOverride,
                setupFunc,
                nullptr,
                false,
                validationRules,
                true
                )
        {
        }

        ValidateTreeParams(
            Platform::String^ subTestId,
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc,
            std::function<void()> cleanupFunc,
            bool disableHittestingOnRoot = true,
            bool ignorePopups = false
            )
            : ValidateTreeParams(
                subTestId,
                windowSizeOverride,
                scaleFactorOverride,
                setupFunc,
                cleanupFunc,
                false,
                ref new Platform::String(DefaultUIElementTreeValidationRules),
                disableHittestingOnRoot,
                ignorePopups
                )
        {
        }

        ValidateTreeParams(
            Platform::String^ subTestId,
            wf::Size windowSizeOverride,
            float scaleFactorOverride,
            std::function<xaml_controls::Panel^()> setupFunc,
            std::function<void()> cleanupFunc,
            bool skipHCColorValidationOnMasterMismatch,
            Platform::String^ validationRules,
            bool disableHittestingOnRoot = true,
            bool ignorePopups = false
            )
        {
            SubTestId = subTestId;
            WindowSizeOverride = windowSizeOverride;
            ScaleFactorOverride = scaleFactorOverride;
            SetupFunc = setupFunc;
            CleanupFunc = cleanupFunc;
            SkipHCColorValidationOnMasterMismatch = skipHCColorValidationOnMasterMismatch;
            ValidationRules = validationRules;
            DisableHittestingOnRoot = disableHittestingOnRoot;
            IgnorePopups = ignorePopups;
        }

        Platform::String^ SubTestId;

        wf::Size WindowSizeOverride;
        float ScaleFactorOverride;

        // The SetupFunc function object should configure a visual tree that represents all
        // the states of a control that need to be validated.  It is responsible for setting
        // that tree as the WindowContent.
        std::function<xaml_controls::Panel^()> SetupFunc;
        std::function<void()> CleanupFunc;

        bool SkipHCColorValidationOnMasterMismatch;

        // Set IsHitTestVisible=False on the root visual if this is true.
        bool DisableHittestingOnRoot;

        Platform::String^ ValidationRules;
        
        bool IgnorePopups = false;
    };

} } } } }
