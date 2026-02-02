XAML Custom Predicate
===

# Background

This spec provides WinUI3 XAML applications with the ability to define custom predicates. This extends the concept from
the pre-existing [XAML conditionals](https://learn.microsoft.com/windows/uwp/debug-test-perf/conditional-xaml).

XAML conditionals allow you to conditionally include or exclude markup based on runtime conditions.
The platform provides built-in predicates like `IsApiContractPresent`, `IsTypePresent`, and `IsPropertyPresent`.

Custom predicates address scenarios where you need conditional XAML based on:
- Application-specific feature flags
- Custom device capabilities
- Business logic conditions
- Configuration settings
- Any other runtime condition specific to your application

By implementing the `IXamlPredicate` interface, you can create predicates that work seamlessly with XAML's conditional namespace syntax,
evaluated at runtime.

# Conceptual pages (How To)

The guidance in the below examples can be followed by developers for using custom predicates in their WinUI3 XAML applications. 
Conditional evaluation for markup can be achieved for elements as well as their attributes. It can also be achieved for user controls, styles,
storyboard, Grid,resources, and more. 

### Remarks
* Any two elements cannot have the same x:Name, even if they are conditionally loaded.
* If multiple conditions are applied to an element attribute, ensure that only one condition evaluates to true at runtime to avoid exceptions.
* Custom predicates are evaluated at runtime, so compiler checks for logical consistency cannot be performed.
* Custom predicates don't work with anything that is derived from 'xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"' namespace. 
  For example: x:uid, x:Key etc.
* For single predicate with same args, the evaluate result is cached. The evaluate result during startup decides the loading of the elements.
  For example, if a TextBlock is conditionally loaded based on a custom predicate with argument "FeatureX" and the predicate evaluates to true during startup,
  the TextBlock will be loaded. Subsequent changes to the condition (e.g., toggling "FeatureX" off) will not affect the loading of the TextBlock.


# API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

## IXamlPredicate interface

IXamlPredicate interface defines a custom predicate that can be used in XAML conditionals. This interface consists of a single method,
Evaluate, which takes a vector of string arguments and returns a boolean value indicating the result of the predicate evaluation. This allows
developers to implement their own logic for determining conditions in XAML based on application-specific requirements. A developer-defined
class implementing this interface can then be referenced in XAML to control the inclusion or exclusion of markup
based on the evaluation result.


Example:
```c#
namespace CustomPredicateNamespace
{
    public class MyCustomPredicate : DependencyObject, Microsoft.UI.Xaml.Markup.IXamlPredicate
    {
        public MyCustomPredicate()
        {

        }
        public bool Evaluate(IReadOnlyList<string> arguments)
        {
            var arg = arguments.FirstOrDefault();
            if (arg == "ConditionOne" || arg == "ConditionThree" || arg == "ConditionDerived" )
                return true;
            else
                return false;
        }
    }
} 
```


```xaml
<?xml version="1.0" encoding="utf-8"?>
<Window
    x:Class="YourApplicationNamespace.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
    xmlns:local="using:YourApplicationNamespace"
    xmlns:predicate="using:CustomPredicateNamespace"
    xmlns:condition1="http://schemas.microsoft.com/winfx/2006/xaml/presentation?predicate:MyCustomPredicate(ConditionOne)"
    xmlns:condition2="http://schemas.microsoft.com/winfx/2006/xaml/presentation?predicate:MyCustomPredicate(ConditionTwo)"
    xmlns:condition3="http://schemas.microsoft.com/winfx/2006/xaml/presentation?predicate:MyCustomPredicate(ConditionThree)"
    xmlns:condition4="http://schemas.microsoft.com/winfx/2006/xaml/presentation?predicate:MyCustomPredicate(ConditionFour)"
    xmlns:conditionderived="using:YourApplicationDerivedUserControlNamespace?predicate:MyCustomPredicate(ConditionDerived)"
    mc:Ignorable="d"
    Title="YourApplicationNamespace">
    <Grid>
    <Grid.Resources>
        <!-- Style for Buttons -->
        <Style x:Key="CustomButtonStyle" TargetType="Button">
            <condition1:Setter Property="Background" Value="PaleVioletRed"/>
            <condition2:Setter Property="Background" Value="DodgerBlue"/>
            <Setter Property="Foreground" condition1:Value="Black" condition2:Value="White"/>
            <Setter Property="Padding" Value="20,10"/>
            <Setter Property="CornerRadius" Value="5"/>
        </Style>
    </Grid.Resources>
        <StackPanel>
            <condition1:TextBlock Text="Hello, I am loaded if condition one is evaluated to true." x:Name="TextBlock1"/>
            <condition2:TextBlock Text="Hello, I am loaded if condition two is evaluated to true." x:Name="TextBlock2"/>
            <Button Content="My background is red if ConditionOne is true else it is purple if ConditionTwo is true. Both cannot be true at the same time." condition1:Background="Red" condition2:Background="Purple" x:Name="MyButton1" Click="OnClick"/>
            <Button Content="I am getting my style from CustomButtonStyle based on conditional evaluation." Style="{StaticResource CustomButtonStyle}" x:Name="MyButton2" Click="OnClick"/>
            <conditionderived:ButtonDerived/>
        </StackPanel>
    </Grid>
</Window>

```

In the above example, a custom predicate `MyCustomPredicate` is defined in the `CustomPredicateNamespace` namespace.
While using custom conditionals, developers need to be cautious that more than one condition for element attributes like
Background in the above example should not evaluate to true at the same time, as it may lead to unexpected behavior
and cause an exception at runtime. Since the evaluation happens at runtime, compiler doesn't perform any sanity checks for such scenarios.

More examples:

```xaml
<Grid.Resources>
     <!-- ControlTemplate for Button -->
     <!-- Condition1 for the custom template -->
     <condition1:ControlTemplate x:Key="CustomButtonTemplate" TargetType="Button">
         <Grid>
             <VisualStateManager.VisualStateGroups>
                 <VisualStateGroup x:Name="CommonStates">
                     <VisualState x:Name="Normal"/>
                     <VisualState x:Name="PointerOver">
                         <condition3:Storyboard>
                             <ObjectAnimationUsingKeyFrames Storyboard.TargetName="RootBorder" Storyboard.TargetProperty="Background">
                                 <DiscreteObjectKeyFrame KeyTime="0" Value="{ThemeResource ButtonBackgroundPointerOver}"/>
                             </ObjectAnimationUsingKeyFrames>
                             <ObjectAnimationUsingKeyFrames Storyboard.TargetName="ContentPresenterElement" Storyboard.TargetProperty="Foreground">
                                 <condition4:DiscreteObjectKeyFrame KeyTime="0" Value="Red"/>
                             </ObjectAnimationUsingKeyFrames>
                         </condition3:Storyboard>
                     </VisualState>
                     <VisualState x:Name="Pressed">
                         <Storyboard>
                             <ObjectAnimationUsingKeyFrames Storyboard.TargetName="RootBorder" Storyboard.TargetProperty="Background">
                                 <DiscreteObjectKeyFrame KeyTime="0" Value="{ThemeResource ButtonBackgroundPressed}"/>
                             </ObjectAnimationUsingKeyFrames>
                         </Storyboard>
                     </VisualState>
                 </VisualStateGroup>
             </VisualStateManager.VisualStateGroups>
             <Border x:Name="RootBorder"
             Background="{TemplateBinding Background}"
             BorderBrush="{TemplateBinding BorderBrush}"
             BorderThickness="{TemplateBinding BorderThickness}"
             CornerRadius="8"
             Padding="{TemplateBinding Padding}">
                 <ContentPresenter x:Name="ContentPresenterElement" HorizontalAlignment="{TemplateBinding HorizontalContentAlignment}"
                         VerticalAlignment="{TemplateBinding VerticalContentAlignment}"/>
             </Border>
         </Grid>
     </condition1:ControlTemplate>
     <!-- Condition2 for the custom template -->
     <condition2:ControlTemplate x:Key="CustomButtonTemplate" TargetType="Button">
         <Grid>
             <VisualStateManager.VisualStateGroups>
                 <VisualStateGroup x:Name="CommonStates">
                     <VisualState x:Name="Normal"/>
                     <VisualState x:Name="PointerOver">
                         <condition3:Storyboard>
                             <ObjectAnimationUsingKeyFrames Storyboard.TargetName="RootBorder" Storyboard.TargetProperty="Background">
                                 <DiscreteObjectKeyFrame KeyTime="0" Value="{ThemeResource ButtonBackgroundPointerOver}"/>
                             </ObjectAnimationUsingKeyFrames>
                             <ObjectAnimationUsingKeyFrames Storyboard.TargetName="ContentPresenterElement" Storyboard.TargetProperty="Foreground">
                                 <condition4:DiscreteObjectKeyFrame KeyTime="0" Value="Black"/>
                             </ObjectAnimationUsingKeyFrames>
                         </condition3:Storyboard>
                     </VisualState>
                     <VisualState x:Name="Pressed">
                         <Storyboard>
                             <ObjectAnimationUsingKeyFrames Storyboard.TargetName="RootBorder" Storyboard.TargetProperty="Background">
                                 <DiscreteObjectKeyFrame KeyTime="0" Value="{ThemeResource ButtonBackgroundPressed}"/>
                             </ObjectAnimationUsingKeyFrames>
                         </Storyboard>
                     </VisualState>
                 </VisualStateGroup>
             </VisualStateManager.VisualStateGroups>
             <Border x:Name="RootBorder"
             Background="{TemplateBinding Background}"
             BorderBrush="{TemplateBinding BorderBrush}"
             BorderThickness="{TemplateBinding BorderThickness}"
             CornerRadius="8"
             Padding="{TemplateBinding Padding}">
                 <ContentPresenter x:Name="ContentPresenterElement" HorizontalAlignment="{TemplateBinding HorizontalContentAlignment}"
                         VerticalAlignment="{TemplateBinding VerticalContentAlignment}"/>
             </Border>
         </Grid>
     </condition2:ControlTemplate>
</Grid.Resources>
```

## IXamlPredicate members

| Name | Description |
|-|-|
| Evaluate | Evaluate method accepts a vector of string arguments and returns a boolean value indicating the result of the predicate evaluation. |

# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml.Markup
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 10)]
    [webhosthidden]
    interface IXamlPredicate 
    {
        Boolean Evaluate(Windows.Foundation.Collections.IVectorView<String> arguments);
    };
}
```
