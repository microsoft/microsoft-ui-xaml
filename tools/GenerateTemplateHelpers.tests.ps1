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
    It 'Should Write the provided string into a new file'{
        $file = WriteTemplateToTempFile test
        Get-Content $file | Should -Be "test"
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