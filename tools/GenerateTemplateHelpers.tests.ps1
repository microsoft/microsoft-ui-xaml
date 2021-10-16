Import-Module .\GenerateTemplateHelpers.ps1 -Force

Describe 'GetStyleFromFile'{
    It 'should throw when file does not exist'{
        { GetTemplateFromFile NoSuchFile ControlName } | Should -Throw
    }

    It 'should Throw if no content'{
        Mock Get-Item {
            New-MockObject -Type 'Object'
        }
        {GetTemplateFromFile FileName ControlName} | Should -Throw
    }

    It 'should throw if the file doesnt contain a template for the control'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
            <Setter Property="Template" TargetType="local:InfoBadge">
                <Setter.Value>
                    <ControlTemplate TargetType="local:NotInfoBadge">
"@ 
        {GetTemplateFromFile $localTestPath ControlName} | Should -Throw
    }

    It 'should throw if the file doesnt contain an end to the template for the control'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
            <Setter Property="Template" TargetType="local:InfoBadge">
                <Setter.Value>
                    <ControlTemplate TargetType="local:InfoBadge">

"@ 
        {GetTemplateFromFile $localTestPath InfoBadge} | Should -Throw
    }

    It 'Should return a template from a well formed file'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
<ControlTemplate TargetType="local:InfoBadge">
Content
</ControlTemplate>

"@ 
        GetTemplateFromFile $localTestPath InfoBadge | Should -Be  @('<ControlTemplate TargetType="local:InfoBadge">', 'Content', '</ControlTemplate>')
    }
}

Describe 'WriteToTempFile'{
    It 'Should Write the provided strings into a new file'{
        $file = WriteToTempFile @('test1', 'test2', 'test3')
        Get-Content $file | Should -Be  @('test1', 'test2', 'test3')
    }

    
    It 'Should allow empty string'{
        $file = WriteToTempFile @('test1', '', 'test3')
        Get-Content $file | Should -Be  @('test1', '', 'test3')
    }
}

Describe 'GetVisualStateGroupsFromTemplate'{
    It 'Should return nothing if no visual state groups'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
            <Setter Property="Template" TargetType="local:InfoBadge">
                <Setter.Value>
                    <ControlTemplate TargetType="local:InfoBadge">

"@ 
        GetVisualStateGroupsFromTemplate $localTestPath | Should -BeNullOrEmpty
    }

    It 'Should return a multi line string for one visual state group'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
<ControlTemplate TargetType="local:InfoBadge">
<VisualStateGroup x:Name="DisplayKindStates">
Content
</VisualStateGroup>
</ControlTemplate>

"@ 
        (GetVisualStateGroupsFromTemplate $localTestPath) | Should -Be @('<VisualStateGroup x:Name="DisplayKindStates">', 'Content', '</VisualStateGroup>')
    }

    It 'Should return a collection of multi line string for many visual state group'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
<ControlTemplate TargetType="local:InfoBadge">
<VisualStateGroup x:Name="DisplayKindStates">
Content
</VisualStateGroup>
<VisualStateGroup x:Name="DisplayKindStates2">
Content
</VisualStateGroup>
<VisualStateGroup x:Name="DisplayKindStates3">
Content
</VisualStateGroup>
</ControlTemplate>

"@ 
        GetVisualStateGroupsFromTemplate $localTestPath | Should -Be @(@('<VisualStateGroup x:Name="DisplayKindStates">', 'Content', '</VisualStateGroup>'), @('<VisualStateGroup x:Name="DisplayKindStates2">', 'Content', '</VisualStateGroup>'), @('<VisualStateGroup x:Name="DisplayKindStates3">', 'Content', '</VisualStateGroup>'))
    }

    It 'Should throw if unqual number of open and closes'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
<ControlTemplate TargetType="local:InfoBadge">
<VisualStateGroup x:Name="DisplayKindStates">
Content
</VisualStateGroup>
<VisualStateGroup x:Name="DisplayKindStates2">
</ControlTemplate>

"@ 
        {GetVisualStateGroupsFromTemplate $localTestPath} | Should -Throw
    }

    It 'Should throw if VisualStateGroupIsNotClosed'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
<ControlTemplate TargetType="local:InfoBadge">
<VisualStateGroup x:Name="DisplayKindStates">
Content
</ControlTemplate>

"@ 
        {GetVisualStateGroupsFromTemplate $localTestPath} | Should -Throw
    }

    It 'Should throw if VisualStateGroupClosedWithoutOpening'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value @"
<ControlTemplate TargetType="local:InfoBadge">
Content
</VisualStateGroup>
</ControlTemplate>

"@ 
        {GetVisualStateGroupsFromTemplate $localTestPath} | Should -Throw
    }
}


Describe 'ExtractName'{
    It 'Should return nothing if VisualStateGroup has no name'{
        $VisualStateGroup = '<VisualStateGroup>'
        ExtractName $VisualStateGroup | Should -BeNullOrEmpty
    }

    It 'Should return the name if VisualStateGroup has a name'{
        $VisualStateGroup = '<VisualStateGroup  x:Name="DisplayKindStates">'
        ExtractName $VisualStateGroup | Should -Be "DisplayKindStates"
    }
}

Describe 'GetVisualStateNamesFromVisualStateGroup'{
    It 'Should return every name in the visual state group'{
        $VisualStateGroup = @('<VisualStateGroup x:Name="DisplayKindStates">',
        '<VisualState x:Name="Dot"/>',
        '<VisualState x:Name="Icon">',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="FontIcon">',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeFontIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="Value">',
            '<VisualState.Setters>',
                '<Setter Target="ValueTextBlock.Visibility" Value="Visible"/>',
                '<Setter Target="ValueTextBlock.Margin" Value="{ThemeResource ValueInfoBadgeTextMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
    '</VisualStateGroup>')
    
    GetVisualStateNamesFromVisualStateGroup $VisualStateGroup | Should -Be @('Dot', 'Icon', 'FontIcon', 'Value')
    }

    It 'Should allow empty strings in VisualStateGroup'{
        $VisualStateGroup = @('<VisualStateGroup x:Name="DisplayKindStates">',
        '<VisualState x:Name="Dot"/>',
        '<VisualState x:Name="Icon">',
        '',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="FontIcon">',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeFontIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="Value">',
            '<VisualState.Setters>',
                '<Setter Target="ValueTextBlock.Visibility" Value="Visible"/>',
                '<Setter Target="ValueTextBlock.Margin" Value="{ThemeResource ValueInfoBadgeTextMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
    '</VisualStateGroup>')
    
    GetVisualStateNamesFromVisualStateGroup $VisualStateGroup | Should -Be @('Dot', 'Icon', 'FontIcon', 'Value')
    }
}

Describe 'OutputBoilerPlate'{
    It 'Should throw if output file does not exist'{
        {OutputBoilerPlate 'NotAFile'} | Should -Throw
    }

    It 'Should write to file if it exists'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "Previous Content"
        OutputBoilerPlate $localTestPath
        Get-Content $localTestPath | Should -be @('Previous Content', '// Copyright (c) Microsoft Corporation. All rights reserved.', '// Licensed under the MIT License. See LICENSE in the project root for license information.', '// Generated by microsoft-ui-xaml/tools/GenerateTemplateHelpers.ps1', '#include "pch.h"', '#include "common.h"')
    }
}

Describe 'OutputEnumClass'{
    It 'Should throw if output file does not exist'{
        {OutputEnumClass 'NotAFile' 'EnumName' @('Val1', 'Val2', 'Val3')} | Should -throw
    }

    It 'Should throw if enum name is empty'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "test"
        {OutputEnumClass $localTestPath '' @('Val1', 'Val2', 'Val3')} | Should -throw
    }

    It 'Should throw if enum value names is empty'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "test"
        {OutputEnumClass $localTestPath 'DisplayKindStates' @()} | Should -throw
    }

    It 'Should output EnumClass'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "Previous Content"
        OutputEnumClass $localTestPath 'EnumName' @('Val1', 'Val2', 'Val3')
        Get-Content $localTestPath | Should -be @('Previous Content', '', 'enum class EnumName', '{', '    Val1,', '    Val2,', '    Val3', '};')
    }
}

Describe 'OutputNamespaceBoilerPlate'{
    It 'Should throw if output file does not exist'{
        {OutputNamespaceBoilerPlate 'NotAFile' 'ClassName'} | Should -throw
    }

    It 'Should output Namespace Boilerplate'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "Previous Content"
        OutputNamespaceBoilerPlate $localTestPath 'ClassName'
        Get-Content $localTestPath | Should -be @('Previous Content', '', 'namespace ClassName', '{')
    }
}

Describe 'OutputVisualStateGroupBlock'{
    It 'Should throw if output file does not exist'{
        {OutputVisualStateGroupBlock 'NotAFile' 'EnumName' @('Val1', 'Val2', 'Val3')} | Should -throw
    }

    It 'Should throw if enum name is empty'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "test"
        {OutputVisualStateGroupBlock $localTestPath '' @('Val1', 'Val2', 'Val3')} | Should -throw
    }

    It 'Should throw if enum value names is empty'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "test"
        {OutputVisualStateGroupBlock $localTestPath 'DisplayKindStates' @()} | Should -throw
    }

    It 'Should output a VisualStateGroup region'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "Previous Content"
        OutputVisualStateGroupBlock $localTestPath 'InfoBadgeDisplayKindState' @('Dot', 'Icon', 'Value')
        Get-Content $localTestPath | Should -be @(
             'Previous Content',
             '',
             '#pragma region InfoBadgeDisplayKindState', 
             '    static winrt::hstring ToString(InfoBadgeDisplayKindState state)',
             '    {',
             '        switch (state)',
             '        {',
             '        case InfoBadgeDisplayKindState::Dot:',
             '            return L"Dot";',
             '        case InfoBadgeDisplayKindState::Icon:',
             '            return L"Icon";',
             '        case InfoBadgeDisplayKindState::Value:',
             '            return L"Value";',
             '        default:',
             '            return L"";',
             '        }',
             '    }',
             ''
             '    static bool GoToState(const winrt::Control& control, InfoBadgeDisplayKindState state, bool useTransitions = true)', 
             '    {', 
             '        return winrt::VisualStateManager::GoToState(control, ToString(state), useTransitions);', 
             '    }', 
             '#pragma endregion')
    }
}

describe GetNamedTemplatePartsFromTemplate{
    It 'Should ignore VisualStateGroups and VisualStates'{
        $template = @('<VisualStateGroup x:Name="DisplayKindStates">',
        '<VisualState x:Name="Dot"/>',
        '<VisualState x:Name="Icon">',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" x:Name="IAmASetter" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="FontIcon">',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeFontIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="Value">',
            '<VisualState.Setters>',
                '<Setter Target="ValueTextBlock.Visibility" Value="Visible"/>',
                '<Setter Target="ValueTextBlock.Margin" Value="{ThemeResource ValueInfoBadgeTextMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
    '</VisualStateGroup>'
    '<Grid x:Name="RootGrid"/>')

    GetNamedTemplatePartsFromTemplate $template | Should -Be @('IAmASetter', 'RootGrid')
    }

    It 'Should Allow VisualStateGroups With Empty Strings'{
        $template = @('<VisualStateGroup x:Name="DisplayKindStates">',
        '<VisualState x:Name="Dot"/>',
        '<VisualState x:Name="Icon">',
        '',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" x:Name="IAmASetter" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="FontIcon">',
            '<VisualState.Setters>',
                '<Setter Target="IconPresenter.Visibility" Value="Visible"/>',
                '<Setter Target="IconPresenter.Margin" Value="{ThemeResource IconInfoBadgeFontIconMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
        '<VisualState x:Name="Value">',
            '<VisualState.Setters>',
                '<Setter Target="ValueTextBlock.Visibility" Value="Visible"/>',
                '<Setter Target="ValueTextBlock.Margin" Value="{ThemeResource ValueInfoBadgeTextMargin}"/>',
            '</VisualState.Setters>',
        '</VisualState>',
    '</VisualStateGroup>'
    '<Grid x:Name="RootGrid"/>')

    GetNamedTemplatePartsFromTemplate $template | Should -Be @('IAmASetter', 'RootGrid')
    }
}

Describe OutputNamedTemplatePartsRegion{
    It 'Should throw if output file does not exist'{
        {OutputNamedTemplatePartsRegion 'NotAFile' 'EnumName' @('Val1', 'Val2', 'Val3')} | Should -throw
    }

    It 'Should throw if Control Name is empty'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "test"
        {OutputNamedTemplatePartsRegion $localTestPath '' @('Val1', 'Val2', 'Val3')} | Should -throw
    }

    It 'Should write nothing if named template parts is empty'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "Previous Content"
        OutputNamedTemplatePartsRegion $localTestPath 'DisplayKindStates' @() 
        Get-Content $localTestPath | Should -be @('Previous Content')
    }

    It 'Should output a NamedTemplateParts region'{
        $localTestPath = "TestDrive:\test2.txt"
        Set-Content $localTestPath -value "Previous Content"
        OutputNamedTemplatePartsRegion $localTestPath 'InfoBadge' @('RootGrid', 'IconPresenter', 'Test3Value')
        Get-Content $localTestPath | Should -be @(
            'Previous Content',
             '',
             '#pragma region NamedTemplateParts', 
             '    static winrt::hstring ToString(InfoBadgeNamedTemplatePart part)',
             '    {',
             '        switch (part)',
             '        {',
             '        case InfoBadgeNamedTemplatePart::RootGrid:',
             '            return L"RootGrid";',
             '        case InfoBadgeNamedTemplatePart::IconPresenter:',
             '            return L"IconPresenter";',
             '        case InfoBadgeNamedTemplatePart::Test3Value:',
             '            return L"Test3Value";',
             '        default:'
             '            return L"";'
             '        }',
             '    }',
             ''
             '    template<typename WinRTReturn>'
             '    WinRTReturn GetTemplatePart(tracker_ref<WinRTReturn>& tracker, InfoBadgeNamedTemplatePart namedTemplatePart, const winrt::IControlProtected& control)'
             '    {'
             '        auto const part = GetTemplateChildT<WinRTReturn>(ToString(namedTemplatePart), control);'
             '        tracker.set(part);'
             '        return part;'
             '    }'
             '#pragma endregion')
    }
}