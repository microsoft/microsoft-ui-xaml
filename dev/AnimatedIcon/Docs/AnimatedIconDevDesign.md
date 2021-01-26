# AnimatedIcon Dev Design
This doc details the technical challenges and specifications of the animated icon control.

## IDL Definition
```
[WUXC_VERSION_PREVIEW]
[webhosthidden]
interface IRichAnimatedVisualSource
{
    IAnimatedVisual TryCreateAnimatedVisual(Windows.UI.Composition.Compositor compositor);
    Windows.Foundation.Collections.IMapView<String, Double> Markers { get; };
    void SetColorProperty(String propertyName, Windows.UI.Color value);
};

[WUXC_VERSION_PREVIEW]
[webhosthidden]
unsealed runtimeclass AnimatedIcon : Windows.UI.Xaml.Controls.IconElement
{
    AnimatedIcon();

    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    IRichAnimatedVisualSource Source{ get; set; };

    [MUX_DEFAULT_VALUE("Normal")]
    [MUX_PROPERTY_CHANGED_CALLBACK_METHODNAME("OnAnimatedIconStatePropertyChanged")]
    static Windows.UI.Xaml.DependencyProperty StateProperty{ get; };
    static void SetState(Windows.UI.Xaml.DependencyObject object, String value);
    static String GetState(Windows.UI.Xaml.DependencyObject object);

    static Windows.UI.Xaml.DependencyProperty SourceProperty{ get; };
}
```

#### Inherited Animated Icon API
```
Brush Foreground {get;set;} // only accepts solidcolorbrush, no-op otherwise Inherited from IconElement. 
```

### Details
Control templates will not (normally) have animated icons in them. Instead they generally have a content presenter or grid who's content is template bound to the controls Icon property, which is of type IconElement. For this reason, in order to set the animated icon properties via the visual states, the animated icon properties need to be attached properties.  In addition to that, the template won't generally be setting the property on the animated icon itself, instead it will usually be setting it on the parent. So in the properties setter we will want to also set the property on the targets child (the animated icon) if it exists. Additionally, in animated Icon's loaded event we will want to make sure to grab the properties off the parent in case it was set before we loaded.
## Control Template Example
An icon button template which has been updated to support using an animated icon in its icon property.
```xaml
<Button Content="Hello Button" Width="200" Height="40">
    <Button.Style>
        <Style TargetType="Button">
            <Setter Property="Template">
                <Setter.Value>
                    <ControlTemplate TargetType="Button">
                        <Grid>
                            <VisualStateManager.VisualStateGroups>
                                <VisualStateGroup x:Name="CommonStates">
                                    <VisualState x:Name="Normal">
	                                    <VisualState.Setters>
		                                    <Setter Target="Icon.(AnimatedIcon.State)" Value="Normal"/>
	                                    </VisualSTate.Setters>
                                    </VisualState>
                                    <VisualState x:Name="PointerOver">
		                                <VisualState.Setters>
		                                    <Setter Target="Icon.(AnimatedIcon.State)" Value="Hover"/>
	                                    </VisualSTate.Setters>
                                    </VisualState>
                                    <VisualState x:Name="Pressed">
		                                <VisualState.Setters>
		                                    <Setter Target="Icon.(AnimatedIcon.State)" Value="Pressed"/>
	                                    </VisualSTate.Setters>
                                    </VisualState>
                                </VisualStateGroup>
                            </VisualStateManager.VisualStateGroups>

                            <ContentPresenter
                                x:Name="ContentPresenter"
                                Background="{TemplateBinding Background}"
                                BorderBrush="{TemplateBinding BorderBrush}"
                                BorderThickness="{TemplateBinding BorderThickness}"
                                Content="{TemplateBinding Content}"
                                ContentTemplate="{TemplateBinding ContentTemplate}"
                                ContentTransitions="{TemplateBinding ContentTransitions}"
                                Padding="{TemplateBinding Padding}"
                                HorizontalContentAlignment="{TemplateBinding HorizontalContentAlignment}"
                                VerticalContentAlignment="{TemplateBinding VerticalContentAlignment}"
                                AutomationProperties.AccessibilityView="Raw" />

                            <Grid x:Name="Icon" HorizontalAlignment="Right" Content="{TemplateBinding Icon}/>
                        </Grid>
                    </ControlTemplate>
                </Setter.Value>
            </Setter>
        </Style>
    </Button.Style>
</Button>
```

## Requirements
#### System Xaml requirements:
- None

#### Lotti Gen requirements:
- LottieGen adds a method for colors that take strings: `void SetColorProperty(String propertyName, Windows.UI.Color value);`
- LottieGen adds a method to convert a marker name into a progress value: `Windows.Foundation.Collections.IMapView<String, Double> Markers { get; };`
- LottieGen output implements IRichAnimatedVisualSource.

#### Designer requirements:
- Provide markers using well known names to specify the start and end of the control states.
- Ideally we would publish a library of animated icons which have the appropriate state markers.

## Conclusions
#### Benefits:
- Animation segments are contained within the animated icon, while the parent control drives the transitioning between states.
- Has easy integration with VSM.
- Is expandable beyond the common states, for usage in things like checkbox.

Downsides:
- A little bit magic that setting a string to a known value would play an animation.
- If the creator of the lottie animation didn't provide markers then the developer would have to hand edit the Lottie JSON or lottie gen output to add the markers themselves.
- There is only one state property. What if we want to use animated icon with a control which has multiple orthogonal state groups 

Red Flags:
- Adobe After Effects does not support multiple markers being placed on the same frame. I see two options to resolve this:
	- Ask Designer to add all of the state names to a single markers comment, ie: "RestToHoverStart,HoverToRestEnd". This would require additional work from lottie gen.
	- Do not allow designers to have animations which share end points.
