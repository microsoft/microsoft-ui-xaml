# AnimatedIcon State Property Changed logic
AnimatedIcon is being designed to provide developers and designers a method for specifying how an AnimatedIcon should respond to control state changes. This doc will describe the extent of that problem and our proposed solution. To make the discussion more concrete we will focus on an example using ToggleSwitch. However this is of course extensible to other controls and states.

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
|  NormalDragging -> PointerOverDragging | NormalDragging -> NormalOn  |  
|  NormalDragging -> PointerOverDragging | NormalDragging -> NormalOff |   
|  NormalDragging -> PointerOverDragging | PointerOverDragging -> PointerOverOn | 
|  NormalOn -> PointerOverOn | PointerOverDragging -> PointerOverOff  | 
|  NormalOn -> PressedOn |  PressedDragging -> PressedOff | 
|  NormalOn -> DisabledOn |  PressedDragging -> PressedOn | 
|  NormalOff -> PointerOverOff |  DisabledDragging -> DisabledOff | 
|  NormalOff -> PressedOff |  DisabledDragging -> DisabledOn | 
|  NormalOff -> DisabledOff |   | 
|   |   | 
|  PointerOverDragging -> NormalDragging | NormalOff -> NormalOn  | 
|  PointerOverDragging -> PressedDragging | NormalOff -> NormalDragging  | 
|  PointerOverDragging -> DisabledDragging | PointerOverOff -> PointerOverOn  | 
|  PointerOverOn -> NormalOn | PointerOverOff -> PointerOverDragging  | 
|  PointerOverOn -> PressedOn | PressedOff -> PressedOn  | 
|  PointerOverOn -> DisabledOn | PressedOff -> PressedDragging  | 
|  PointerOverOff -> NormalOff | DisabledOff -> DisabledOn  | 
|  PointerOverOff -> PressedOff | DisabledOff -> DisabledDragging  | 
|  PointerOverOff -> DisabledOff |   | 
|   |   |
|  PressedDragging -> NormalDragging | NormalOn -> NormalDragging  | 
|  PressedDragging -> PointerOverDragging   | NormalOn -> NormalOff  | 
|  PressedDragging -> DisabledDragging       | PointerOverOn -> PointerOverDragging  | 
|  PressedOn -> NormalOn   | PointerOverOn -> PointerOverOff  | 
|  PressedOn -> PointerOverOn   | PressedOn -> PressedDragging  | 
|  PressedOn -> DisabledOn      | PressedOn -> PressedOff  | 
|  PressedOff -> NormalOff  | DisabledOn -> DisabledDragging  | 
|  PressedOff -> PointerOverOff  | DisabledOn -> DisabledOff  | 
|  PressedOff -> DisabledOff |   | 
|   |   | 
|  DisabledDragging -> NormalDragging |   | 
|  DisabledDragging -> PointerOverDragging |   | 
|  DisabledDragging -> PressedDragging |   | 
|  DisabledOn -> NormalOn |   | 
|  DisabledOn -> PointerOverOn |   | 
|  DisabledOn -> PressedOn |   | 
|  DisabledOff -> NormalOff |   | 
|  DisabledOff -> PointerOverOff |   | 
|  DisabledOff -> PressedOff |   | 


In order to specify an animation for every state transitions the Lottie author will need to specify 120 markers with the following format:

|  [FromState]To[ToState]Start	 |       [FromState]To[ToState]End   |
|:---:|:---:|
| NormalDraggingToPointerOverDraggingStart | PointerOverDraggingToNormalDraggingEnd |
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
There are two use cases I see here. First the designer doesn't care about one of the orthogonal state groups. In the proposed states maybe they only have animations for the ToggleSates, not the CommonStates.The designer should not have to specify NormalOn -> NormalOff, On -> Off should be sufficient. On->Off would also cover PointerOverOn->PointerOverOff PressedOn->PressedOff and DisabledOn->DisabledOff if those weren't seperately specified.

Second, the designer wants state transitons to play in reverse to undo a transition. For example if the designer only specifies NormalOn->NormalOff then in the NormalOff->NormalOn transition we would play the NormalOn->NormalOff in reverse.

##### 4) Don't depend on the order the state groups are specified in.
 When there are multiple orthoginal state groups we don't want to fail if the designer or developer applies them in the wrong order. For instance if the designer specifies OnNormalToOffNormalStart and OnNormalToOffNormalEnd but the developer expects NormalOnToNormalOffStart and NormalOnToNormalOffEnd we should still function.

##### 5) If only the start of the end marker is found, we should not fail.
If the designer forgets (or purposefully omits) one of the start or end markers, we should do something that makes sense.  I purpose that we cut to the position of the maker we did find. For example if NormalOnToNormalOffStart is 0.1 and NormalOnToNormalOffEnd is undefined we would cut to 0.1

## Algorithm 
Given the above requirements I purpose the following algorithm. We will use the example of StateGroup1 being changed from Normal to PointerOver while StateGroup2 is set to Off to make the more concrete
### Example:
OnStateGroup1PropertyChanged
1. Check for `NormalOffToPointerOverOffStart` and `NormalOffToPointerOverOffEnd`. If either is found return[^1].
2. Check for `OffNormalToOffPointerOverStart` and `OffNormalToOffPointerOverEnd`. If either is found return[^1].
3. Check for `PointerOverOffToNormalOffStart` and `PointerOverOffToNormalOffEnd`. If *both* are found return the segment in reverse.
4. Check for `OffPointerOverToOffNormalStart` and `OffPointerOverToOffNormalEnd`. If *both* are found return the segment in reverse.
5. Check for `NormalToPointerOverStart` and `NormalToPointerOverEnd`. If either is found return[^1].
6. Check for `PointerOverToNormalStart` and `PointerOverToNormalEnd`. If *both* is found return the segment in reverse.
7. Return a hard cut to position 0.0.

[^1]: If only one of the paired markers is found we would return a hard cut to the position specified by the single marker.

### Formalized algorithm with 3 state groups:
OnStateGroup1PropertyChanged
1. Check for `[PrevStateGroup1][StateGroup2][StateGroup3]To[NewStateGroup1][StateGroup2][StateGroup3]Start` and `[PrevStateGroup1][StateGroup2][StateGroup3]To[NewStateGroup1][StateGroup2][StateGroup3]End`. If either is found return[^1].
2. Check for `[PrevStateGroup1][StateGroup3][StateGroup2]To[NewStateGroup1][StateGroup3][StateGroup2]Start` and `[PrevStateGroup1][StateGroup3][StateGroup2]To[NewStateGroup1][StateGroup3][StateGroup2]End`. If either is found return[^1].
3. Check for `[StateGroup2][PrevStateGroup1][StateGroup3]To[StateGroup2][NewStateGroup1][StateGroup3]Start` and `[StateGroup2][PrevStateGroup1][StateGroup3]To[StateGroup2][NewStateGroup1][StateGroup3]End`. If either is found return[^1].
4. Check for `[StateGroup2][StateGroup3][PrevStateGroup1]To[StateGroup2][StateGroup3][NewStateGroup1]Start` and `[StateGroup2][StateGroup3][PrevStateGroup1]To[StateGroup2][StateGroup3][NewStateGroup1]End`. If either is found return[^1].
5. Check for `[StateGroup3][PrevStateGroup1][StateGroup2]To[StateGroup3][NewStateGroup1][StateGroup2]Start` and `[StateGroup3][PrevStateGroup1][StateGroup2]To[StateGroup3][NewStateGroup1][StateGroup2]End`. If either is found return[^1].
6. Check for `[StateGroup3][StateGroup2][PrevStateGroup1]To[StateGroup3][StateGroup2][NewStateGroup1]Start` and `[StateGroup3][StateGroup2][PrevStateGroup1]To[StateGroup3][StateGroup2][NewStateGroup1]End`. If either is found return[^1].
7. Check for `[NewStateGroup1][StateGroup2][StateGroup3]To[PrevStateGroup1][StateGroup2][StateGroup3]Start` and `[NewStateGroup1][StateGroup2][StateGroup3]To[PrevStateGroup1][StateGroup2][StateGroup3]End`. If *both* are found return the segment in reverse.
8. Check for `[NewStateGroup1][StateGroup3][StateGroup2]To[PrevStateGroup1][StateGroup3][StateGroup2]Start` and `[NewStateGroup1][StateGroup3][StateGroup2]To[PrevStateGroup1][StateGroup3][StateGroup2]End`. If *both* are found return the segment in reverse.
9. Check for `[StateGroup2][NewStateGroup1][StateGroup3]To[StateGroup2][PrevStateGroup1][StateGroup3]Start` and `[StateGroup2][NewStateGroup1][StateGroup3]To[StateGroup2][PrevStateGroup1][StateGroup3]End`. If *both* are found return the segment in reverse.
10. Check for `[StateGroup2][StateGroup3][NewStateGroup1]To[StateGroup2][StateGroup3][PrevStateGroup1]Start` and `[StateGroup2][StateGroup3][NewStateGroup1]To[StateGroup2][StateGroup3][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
11. Check for `[StateGroup3][NewStateGroup1][StateGroup2]To[StateGroup3][PrevStateGroup1][StateGroup2]Start` and `[StateGroup3][NewStateGroup1][StateGroup2]To[StateGroup3][PrevStateGroup1][StateGroup2]End`. If *both* are found return the segment in reverse.
12. Check for `[StateGroup3][StateGroup2][NewStateGroup1]To[StateGroup3][StateGroup2][PrevStateGroup1]Start` and `[StateGroup3][StateGroup2][NewStateGroup1]To[StateGroup3][StateGroup2][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
13. Check for `[PrevStateGroup1][StateGroup2]To[NewStateGroup1][StateGroup2]Start` and `[PrevStateGroup1][StateGroup2]To[NewStateGroup1][StateGroup2]End`. If either is found return[^1].
14. Check for `[StateGroup2][PrevStateGroup1]To[StateGroup2][NewStateGroup1]Start` and `[StateGroup2][PrevStateGroup1]To[StateGroup2][NewStateGroup1]End`. If either is found return[^1].
15. Check for `[PrevStateGroup1][StateGroup3]To[NewStateGroup1][StateGroup3]Start` and `[PrevStateGroup1][StateGroup3]To[NewStateGroup1][StateGroup3]End`. If either is found return[^1].
16. Check for `[StateGroup3][PrevStateGroup1]To[StateGroup3][NewStateGroup1]Start` and `[StateGroup3][PrevStateGroup1]To[StateGroup3][NewStateGroup1]End`. If either is found return[^1].
17. Check for `[NewStateGroup1][StateGroup2]To[PrevStateGroup1][StateGroup2]Start` and `[NewStateGroup1][StateGroup2]To[PrevStateGroup1][StateGroup2]End`. If *both* are found return the segment in reverse.
18. Check for `[StateGroup2][NewStateGroup1]To[StateGroup2][PrevStateGroup1]Start` and `[StateGroup2][NewStateGroup1]To[StateGroup2][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
19. Check for `[NewStateGroup1][StateGroup3]To[PrevStateGroup1][StateGroup3]Start` and `[NewStateGroup1][StateGroup3]To[PrevStateGroup1][StateGroup3]End`. If *both* are found return the segment in reverse.
20. Check for `[StateGroup3][NewStateGroup1]To[StateGroup3][PrevStateGroup1]Start` and `[StateGroup3][NewStateGroup1]To[StateGroup3][PrevStateGroup1]End`. If *both* are found return the segment in reverse.
21. Check for `[PrevStateGroup1]To[NewStateGroup1]Start` and `[PrevStateGroup1]To[NewStateGroup1]End`. If either is found return[^1].
22. Check for `[NewStateGroup1]To[PrevStateGroup1]Start` and `[NewStateGroup1]To[PrevStateGroup1]End`. If *both* are found return the segment in reverse.
24. Return a hard cut to position 0.0.
