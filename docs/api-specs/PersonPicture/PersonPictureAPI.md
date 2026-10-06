# Background

Xaml controls are typically implemented using an element tree that’s defined in a template.  For example the template for a ComboBox control has a TextBlock in it to display the selected value.

Several controls have properties that are only expected to be useful to these templates, and so there’s a common pattern for the control to expose them in a TemplateSettings property.  For example [ComboBox.TemplateSettings](https://docs.microsoft.com/en-us/uwp/api/Windows.UI.Xaml.Controls.ComboBox.TemplateSettings) is used by the ComboBox’s template.  See [this doc page](https://docs.microsoft.com/en-us/windows/uwp/xaml-platform/template-settings-classes) for more details. 

This API update introduces this pattern to the PersonPicture control: a new TemplateSettings property and associated PersonPictureTemplateSettings class.

# Description

Provides calculated values that can be referenced as TemplatedParent sources when defining templates for a PersonPicture control. Not intended for general use.

# API Details

```
namespace Microsoft.UI.Xaml.Controls
{
    [webhosthidden]
    unsealed runtimeclass PersonPicture : Windows.UI.Xaml.Controls.Control
    {
        ...

        PersonPictureTemplateSettings TemplateSettings { get; };

        ...
    }

    [webhosthidden]
    runtimeclass PersonPictureTemplateSettings : Windows.UI.Xaml.DependencyObject
    {
        String EffectiveInitials { get; };
        Windows.UI.Xaml.Media.ImageBrush EffectiveImageBrush { get; };
    }

}
```

# Examples

This is intended for use in the style of the `PersonPicture` control. Sample usage:

```xaml
    <Style TargetType="local:PersonPicture">
        ...
        <Setter Property="Template">
            <Setter.Value>
                <ControlTemplate TargetType="local:PersonPicture">
                    <Grid x:Name="RootGrid">
                        <VisualStateManager.VisualStateGroups>
                            <VisualStateGroup x:Name="CommonStates">
                                <!-- Visual State when a Photo is available for display -->
                                <VisualState x:Name="Photo">
                                    <VisualState.Setters>
                                        <Setter Target="PersonPictureEllipse.Fill" Value="{Binding RelativeSource={RelativeSource TemplatedParent}, Path=TemplateSettings.EffectiveImageBrush}"/>
                                    </VisualState.Setters>
                                </VisualState>
                    ...
                        <TextBlock
                            ...
                            Text="{Binding RelativeSource={RelativeSource TemplatedParent}, Path=TemplateSettings.EffectiveInitials}" />
                        
```

# API Notes

## Class: PersonPicture
| Member Name | Description |
|:- |:--|
| TemplateSettings | Provides calculated values that can be referenced as TemplatedParent sources when defining templates for a PersonPicture control. Not intended for general use. |

## Class: PersonPictureTemplateSettings 
| Member Name | Description |
|:- |:--|
| EffectiveInitials | The initials that should be displayed in the control. Will be empty when image is specified or in group mode. |
| EffectiveImageBrush | The calculated image brush populated with the specified ImageSource. |
