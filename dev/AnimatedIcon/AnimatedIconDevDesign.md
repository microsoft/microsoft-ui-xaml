# AnimatedIcon Dev Design
This doc details the technical challenges and specifications of the animated icon control.

## IDL Definition
```
Interface IRichAnimatedVisualSource : IAnimatedVisualSource
{
    IAnimatedVisual TryCreateAnimatedVisual(Windows.UI.Compositor compositor, out object diagnostics);
    Windows.Foundation.Collections.IMapView<String, Double> Markers { get; };
    void SetColorProperty(String propertyName, Windows.UI.Color value);
}

public class StatefulIcon : IconElement  
{ 
    [MUX_PROPERTY_CHANGED_CALLBACK_METHODNAME("OnStatePropertyChanged")]
    String StateGroup1{get; set;} //CommonStates
    
    [MUX_PROPERTY_CHANGED_CALLBACK_METHODNAME("OnStatePropertyChanged")]
    String StateGroup2{get; set;} //DisabledStates
    
    [MUX_PROPERTY_CHANGED_CALLBACK_METHODNAME("OnStatePropertyChanged")]
    String StateGroup3{get; set;} //SelectedStates
  
    IRichAnimatedVisualSource Source{get;set;} //IAnimatedVisualSource Source {get;set;}
     
    ~~bool IsPlaying {get;} //Needed? If so, do we need events for when the animation has finished?~~
    
    // Colors 
    Brush Foreground {get;set;} // only accepts solidcolorbrush, no-op otherwise Inherited from IconElement. 

    Windows.Foundation.Collections.IMapView<String, Double> AdditionalMarkers { get; };
} 
```
#### Inherited Animated Icon API
Brush Foreground {get;set;} // only accepts solidcolorbrush, no-op otherwise Inherited from IconElement. 

#### Removed Animated Icon API
bool IsPlaying {get;} //Needed? If so, do we need events for when the animation has finished?
Windows.Foundation.Collections.IMapView<String, Double> AdditionalMarkers { get; };

### Details
Control templates will not (normally) have animated icons in them. Instead they generally have a content presenter or grid who's content is template bound to the controls Icon property, which is of type IconElement. For this reason, in order to set the animated icon properties via the visual states, the animated icon properties need to be attached properties.  In addition to that, the template wont generally be setting the property on the animated icon itself, instead it will usually be setting it on the parent. So in the properties setter we will want to also set the property on the targets child (the animated icon) if it exists. Additionally, in animated Icon's loaded event we will want to make sure to grab the properties off the parent in case it was set before we loaded.
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
		                                    <Setter Target="Icon.(AnimatedIcon.StateGroup1)" Value="Normal"/>
	                                    </VisualSTate.Setters>
                                    </VisualState>
                                    <VisualState x:Name="PointerOver">
		                                <VisualState.Setters>
		                                    <Setter Target="Icon.(AnimatedIcon.StateGroup1)" Value="PointerOver"/>
	                                    </VisualSTate.Setters>
                                    </VisualState>
                                    <VisualState x:Name="Pressed">
		                                <VisualState.Setters>
		                                    <Setter Target="Icon.(AnimatedIcon.StateGroup1)" Value="Pressed"/>
	                                    </VisualSTate.Setters>
                                    </VisualState>
                                <VisualStateGroup x:Name="DisabledState"/>
                                    <VisualState x:Name="Enabled">
	                                    <VisualState.Setters>
	                                        <Setter Target="Icon.(AnimatedIcon.StateGroup2)" Value="Enabled"/>
	                                    </VisualSTate.Setters>
                                    </VisualState>
                                    <VisualState x:Name="Disabled">
	                                    <VisualState.Setters>
	                                        <Setter Target="Icon(AnimatedIcon.StateGroup2)" Value="Disabled"/>
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
- Provide markers using well known names to specify the start and end of the enumerated default states.
- Ideally we would publish a library of animated icons which have the appropriate state markers.

## Conclusions
#### Benefits:
- Moves the specification of the start and end of the hover animation, for example, from the parent template to the animated icon itsself.
- Drastically simplifies that 'golden path' syntax for utilizing VSM.

Downsides:
- A little bit magic that setting a string to a known value would play an animation.
- If the creator of the lottie animation didn't provide markers there would be no way to play the animation.
	- Can we add the ability for the developer to add their own in LottieGen?
- Only supports 3(or whatever number we choose) state groups, what if there is a scenario that needs more?

Red Flags:
- Adobe After Effects does not support multiple markers being placed on the same frame. We have some options to resolve this:
	- Ask Designer to add all of the state names to a single markers comment, ie: "RestToHoverStart,HoverToRestEnd".
	- Do not allow designers to have animations which share end points.
	- Utilize the CSS style comment system that lottieGen uses for its theming system to have lottie gen produce something more sophisticated than the map of strings to double, something along the lines for segments.  Simeon says he thinks he likes this idea but is unsure what exactly it would look like. 
