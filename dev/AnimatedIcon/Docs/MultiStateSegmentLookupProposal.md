Animated Icon is initially being published With a single state proprety, which has the benefit of being very simple for state lookups. However it is possible that there is a control that wants to use animated icon across multiple orthogonal visual state groups. This is an issue because it isn't possible to tell what state StateGroup1 is in while StateGroup2 is changing.  If this ends up being required we would likely need to add a second (or even third) state property to animated icon to accomodate these state groups.  This doc describes what that multi state API would look like. To make the discussion more concrete we will focus on an example using ToggleSwitch. However this is of course extensible to other controls and states.

## ToggleSwitch Example
```xml
<ControlTemplate TargetType="ToggleSwitch">
    <VisualStateGroup Name="CommonStates">
        <VisualState Name="Normal" />
        <VisualState Name="PointerOver"/>
        <VisualState Name="Pressed"/>
        <VisualState Name="Disabled"/>
    </VisualStateGroup>
    <VisualStateGroup Name="ToggleStates">
        <VisualState Name="Dragging" />
        <VisualState Name="Off" />
        <VisualState Name="On"/>
    </VisualStateGroup>
</ControlTemplate>
```
 This example has two state groups of 4 and 3 states. This makes for ((3 * 3) * 4)+((4 * 2) * 3)=60 possible transitions:

| StateGroup1 Changes  | StateGroup2 Changes  |
|---|---|
|  NormalDragging -> HoverDragging | NormalDragging -> NormalOn  |  
|  NormalDragging -> HoverDragging | NormalDragging -> NormalOff |   
|  NormalDragging -> HoverDragging | HoverDragging -> HoverOn | 
|  NormalOn -> HoverOn | HoverDragging -> HoverOff  | 
|  NormalOn -> PressedOn |  PressedDragging -> PressedOff | 
|  NormalOn -> DisabledOn |  PressedDragging -> PressedOn | 
|  NormalOff -> HoverOff |  DisabledDragging -> DisabledOff | 
|  NormalOff -> PressedOff |  DisabledDragging -> DisabledOn | 
|  NormalOff -> DisabledOff |   | 
|   |   | 
|  HoverDragging -> NormalDragging | NormalOff -> NormalOn  | 
|  HoverDragging -> PressedDragging | NormalOff -> NormalDragging  | 
|  HoverDragging -> DisabledDragging | HoverOff -> HoverOn  | 
|  HoverOn -> NormalOn | HoverOff -> HoverDragging  | 
|  HoverOn -> PressedOn | PressedOff -> PressedOn  | 
|  HoverOn -> DisabledOn | PressedOff -> PressedDragging  | 
|  HoverOff -> NormalOff | DisabledOff -> DisabledOn  | 
|  HoverOff -> PressedOff | DisabledOff -> DisabledDragging  | 
|  HoverOff -> DisabledOff |   | 
|   |   |
|  PressedDragging -> NormalDragging | NormalOn -> NormalDragging  | 
|  PressedDragging -> HoverDragging   | NormalOn -> NormalOff  | 
|  PressedDragging -> DisabledDragging       | HoverOn -> HoverDragging  | 
|  PressedOn -> NormalOn   | HoverOn -> HoverOff  | 
|  PressedOn -> HoverOn   | PressedOn -> PressedDragging  | 
|  PressedOn -> DisabledOn      | PressedOn -> PressedOff  | 
|  PressedOff -> NormalOff  | DisabledOn -> DisabledDragging  | 
|  PressedOff -> HoverOff  | DisabledOn -> DisabledOff  | 
|  PressedOff -> DisabledOff |   | 
|   |   | 
|  DisabledDragging -> NormalDragging |   | 
|  DisabledDragging -> HoverDragging |   | 
|  DisabledDragging -> PressedDragging |   | 
|  DisabledOn -> NormalOn |   | 
|  DisabledOn -> HoverOn |   | 
|  DisabledOn -> PressedOn |   | 
|  DisabledOff -> NormalOff |   | 
|  DisabledOff -> HoverOff |   | 
|  DisabledOff -> PressedOff |   | 


In order to specify an animation for every state transitions the Lottie author will need to specify 120 markers with the following format:

|  [FromState]To[ToState]Start	 |       [FromState]To[ToState]End   |
|:---:|:---:|
| NormalDraggingToHoverDraggingStart | HoverDraggingToNormalDraggingEnd |
| NormalDraggingToNormalOnStart            | NormalDraggingToNormalOnEnd            |

## Requirements

The above example is obviously extremely verbose. The goal then is to come up with a way for AnimatedIcon to look up animation segments while conforming to these requirements:
1. Allow for the Customization of all 60 transitions, should they be needed
2. Allow the designer to not specify a transition if they don't have any behavior in mind.
3. Allow the designer to specify multiple transitions with the same marker if they have the same animation
4. Don't depend on the order the state groups are specified in.
5. If only the start of the end marker is found, we should not fail.

##### 1) Allow for the Customization of all 60 transitions, should they be needed
This requires that our lookup algorithm check the most verbose state name (i.e. one of the 120 from the example above) before falling back to the other names.

##### 2) Allow the designer to not specify a transition if they don't have any behavior in mind.
This requires that if at the end of the lookup algorithm we have not found a segment of the Lottie animation to play then we should not fail. I am proposing that we do not want to leave the animation in its current state, that might be the end of a pressed animation for example.  Instead I propose we cut to position 0.0 of the Lottie file, which will behave as the default position.

##### 3) Allow the designer to specify multiple transitions with the same marker if they have the same animation 
There are three use cases I see here. First the designer doesn't care about one of the orthogonal state groups. In the proposed states maybe they only have animations for the ToggleSates, not the CommonStates.The designer should not have to specify NormalOn -> NormalOff, On -> Off should be sufficient. On->Off would also cover HoverOn->HoverOff PressedOn->PressedOff and DisabledOn->DisabledOff if those weren't seperately specified.

Second, the designer wants state transitons to play in reverse to undo a transition. For example if the designer only specifies NormalOn->NormalOff then in the NormalOff->NormalOn transition we would play the NormalOn->NormalOff in reverse.

Third, the designer wants to associate a single frame (not an animation) with a state. For example position 1.0 could be the disabled state.  In this case the marker "Disabled" would specifiy that.  When transitioning from Disabled specified in this manor you would either need something like DisabledToNormalStart and/or DisabedToNormalEnd OR a "Normal" marker. 

##### 4) Don't depend on the order the state groups are specified in.
 When there are multiple orthoginal state groups we don't want to fail if the designer or developer applies them in the wrong order. For instance if the designer specifies OnNormalToOffNormalStart and OnNormalToOffNormalEnd but the developer expects NormalOnToNormalOffStart and NormalOnToNormalOffEnd we should still function.

##### 5) If only the start of the end marker is found, we should not fail.
If the designer forgets (or purposefully omits) one of the start or end markers, we should do something that makes sense.  I purpose that we cut to the position of the maker we did find. For example if NormalOnToNormalOffStart is 0.1 and NormalOnToNormalOffEnd is undefined we would cut to 0.1

## Algorithm 
Given the above requirements I purpose the following algorithm. We will use the example of StateGroup1 being changed from Normal to Hover while StateGroup2 is set to Off to make the more concrete
### Example:
OnStateGroup1PropertyChanged
1. Check for `NormalOffToHoverOffStart` and `NormalOffToHoverOffEnd`. If either is found return<sup>1</sup>.
2. Check for `OffNormalToOffHoverStart` and `OffNormalToOffHoverEnd`. If either is found return<sup>1</sup>.
3. Check for `HoverOffToNormalOffStart` and `HoverOffToNormalOffEnd`. If *both* are found return the segment in reverse.
4. Check for `OffHoverToOffNormalStart` and `OffHoverToOffNormalEnd`. If *both* are found return the segment in reverse.
5. Check for `NormalToHoverStart` and `NormalToHoverEnd`. If either is found return<sup>1</sup>.
6. Check for `HoverToNormalStart` and `HoverToNormalEnd`. If *both* is found return the segment in reverse.
8. Check for `HoverOff`.  If it is found return a hard cut to that position.
9. Check for `OffHover`.  If it is found return a hard cut to that position.
10. Check for `Hover`.  If it is found return a hard cut to that position.
11. Return a hard cut to position 0.0.

### Formalized algorithm with 3 state groups:
OnStateGroup1PropertyChanged
1. Check for `[PrevStateGroup1][StateGroup2][StateGroup3]To[NewStateGroup1][StateGroup2][StateGroup3]Start` and `[PrevStateGroup1][StateGroup2][StateGroup3]To[NewStateGroup1][StateGroup2][StateGroup3]End`. If either is found return<sup>1</sup>.
2. Check for `[PrevStateGroup1][StateGroup3][StateGroup2]To[NewStateGroup1][StateGroup3][StateGroup2]Start` and `[PrevStateGroup1][StateGroup3][StateGroup2]To[NewStateGroup1][StateGroup3][StateGroup2]End`. If either is found return<sup>1</sup>.
3. Check for `[StateGroup2][PrevStateGroup1][StateGroup3]To[StateGroup2][NewStateGroup1][StateGroup3]Start` and `[StateGroup2][PrevStateGroup1][StateGroup3]To[StateGroup2][NewStateGroup1][StateGroup3]End`. If either is found return<sup>1</sup>.
4. Check for `[StateGroup2][StateGroup3][PrevStateGroup1]To[StateGroup2][StateGroup3][NewStateGroup1]Start` and `[StateGroup2][StateGroup3][PrevStateGroup1]To[StateGroup2][StateGroup3][NewStateGroup1]End`. If either is found return<sup>1</sup>.
5. Check for `[StateGroup3][PrevStateGroup1][StateGroup2]To[StateGroup3][NewStateGroup1][StateGroup2]Start` and `[StateGroup3][PrevStateGroup1][StateGroup2]To[StateGroup3][NewStateGroup1][StateGroup2]End`. If either is found return<sup>1</sup>.
6. Check for `[StateGroup3][StateGroup2][PrevStateGroup1]To[StateGroup3][StateGroup2][NewStateGroup1]Start` and `[StateGroup3][StateGroup2][PrevStateGroup1]To[StateGroup3][StateGroup2][NewStateGroup1]End`. If either is found return<sup>1</sup>.
7. Check for `[NewStateGroup1][StateGroup2][StateGroup3]To[PrevStateGroup1][StateGroup2][StateGroup3]Start` and `[NewStateGroup1][StateGroup2][StateGroup3]To[PrevStateGroup1][StateGroup2][StateGroup3]End`. If *both* are found return the segment in reverse.
8. Check for `[NewStateGroup1][StateGroup3][StateGroup2]To[PrevStateGroup1][StateGroup3][StateGroup2]Start` and `[NewStateGroup1][StateGroup3][StateGroup2]To[PrevStateGroup1][StateGroup3][StateGroup2]End`. If *both* are found return the segment in reverse.
9. Check for `[StateGroup2][NewStateGroup1][StateGroup3]To[StateGroup2][PrevStateGroup1][StateGroup3]Start` and `[StateGroup2][NewStateGroup1][StateGroup3]To[StateGroup2][PrevStateGroup1][StateGroup3]End`. If *both* are found return the segment in reverse.
10. Check for `[StateGroup2][StateGroup3][NewStateGroup1]To[StateGroup2][StateGroup3][PrevStateGroup1]Start` and `[StateGroup2][StateGroup3][NewStateGroup1]To[StateGroup2][StateGroup3][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
11. Check for `[StateGroup3][NewStateGroup1][StateGroup2]To[StateGroup3][PrevStateGroup1][StateGroup2]Start` and `[StateGroup3][NewStateGroup1][StateGroup2]To[StateGroup3][PrevStateGroup1][StateGroup2]End`. If *both* are found return the segment in reverse.
12. Check for `[StateGroup3][StateGroup2][NewStateGroup1]To[StateGroup3][StateGroup2][PrevStateGroup1]Start` and `[StateGroup3][StateGroup2][NewStateGroup1]To[StateGroup3][StateGroup2][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
13. Check for `[PrevStateGroup1][StateGroup2]To[NewStateGroup1][StateGroup2]Start` and `[PrevStateGroup1][StateGroup2]To[NewStateGroup1][StateGroup2]End`. If either is found return<sup>1</sup>.
14. Check for `[StateGroup2][PrevStateGroup1]To[StateGroup2][NewStateGroup1]Start` and `[StateGroup2][PrevStateGroup1]To[StateGroup2][NewStateGroup1]End`. If either is found return<sup>1</sup>.
15. Check for `[PrevStateGroup1][StateGroup3]To[NewStateGroup1][StateGroup3]Start` and `[PrevStateGroup1][StateGroup3]To[NewStateGroup1][StateGroup3]End`. If either is found return<sup>1</sup>.
16. Check for `[StateGroup3][PrevStateGroup1]To[StateGroup3][NewStateGroup1]Start` and `[StateGroup3][PrevStateGroup1]To[StateGroup3][NewStateGroup1]End`. If either is found return<sup>1</sup>.
17. Check for `[NewStateGroup1][StateGroup2]To[PrevStateGroup1][StateGroup2]Start` and `[NewStateGroup1][StateGroup2]To[PrevStateGroup1][StateGroup2]End`. If *both* are found return the segment in reverse.
18. Check for `[StateGroup2][NewStateGroup1]To[StateGroup2][PrevStateGroup1]Start` and `[StateGroup2][NewStateGroup1]To[StateGroup2][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
19. Check for `[NewStateGroup1][StateGroup3]To[PrevStateGroup1][StateGroup3]Start` and `[NewStateGroup1][StateGroup3]To[PrevStateGroup1][StateGroup3]End`. If *both* are found return the segment in reverse.
20. Check for `[StateGroup3][NewStateGroup1]To[StateGroup3][PrevStateGroup1]Start` and `[StateGroup3][NewStateGroup1]To[StateGroup3][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
21. Check for `[PrevStateGroup1]To[NewStateGroup1]Start` and `[PrevStateGroup1]To[NewStateGroup1]End`. If either is found return<sup>1</sup>.
22. Check for `[NewStateGroup1]To[PrevStateGroup1]Start` and `[NewStateGroup1]To[PrevStateGroup1]End`. If *both* are found return the segment in reverse.
23. Check for `[NewStateGroup1][StateGroup2][StateGroup3]`. If it is found return a hard cut to that position.
24. Check for `[NewStateGroup1][StateGroup3][StateGroup2]`. If it is found return a hard cut to that position.
25. Check for `[StateGroup2][NewStateGroup1][StateGroup3]`. If it is found return a hard cut to that position.
26. Check for `[StateGroup2][StateGroup3][NewStateGroup1]`. If it is found return a hard cut to that position.
27. Check for `[StateGroup3][NewStateGroup1][StateGroup2]`. If it is found return a hard cut to that position.
28. Check for `[StateGroup3][StateGroup2][NewStateGroup1]`. If it is found return a hard cut to that position.
29. Check for `[NewStateGroup1][StateGroup2]`. If it is found return a hard cut to that position.
30. Check for `[StateGroup2][NewStateGroup1]`. If it is found return a hard cut to that position.
31. Check for `[NewStateGroup1][StateGroup3]`. If it is found return a hard cut to that position.
32. Check for `[StateGroup3][NewStateGroup1]`. If it is found return a hard cut to that position.
33. Check for `[NewStateGroup1]`. If it is found return a hard cut to that position.
34. Return a hard cut to position 0.0.



1: If only one of the paired markers is found we would return a hard cut to the position specified by the single marker.
