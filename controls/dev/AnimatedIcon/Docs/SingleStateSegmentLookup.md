Animated Icon is being published with a single state property. While this makes the segment lookup logic simplier than multiple state properties, there is still some complexity which will be outlined here.

## Example
First, we will examine the example of an animated icon with a state property set to "Normal" which then changes to "PointerOver". Here are the steps animated icon does to determine what segment to play.

1. Check the provided IRichAnimatedIconSource's markers for the markers "NormalToPointerOver_Start" and "NormalToPointerOver_End". If both are found play the animation from "NormalToPointerOver_Start" to "NormalToPointerOver_End".
2. If "NormalToPointerOver_Start" is not found but "NormalToPointerOver_End" is found, then we will hard cut to the "NormalToPointerOver_End" position.
3. If "NormalToPointerOver_End" is not found but "NormalToPointerOver_Start" is found, then we will hard cut to the "NormalToPointerOver_Start" position.
4. Check if the provided IRichAnimatedIconSource's markers for the marker "NormalToPointerOver". If it is found, then we will hard cut to the "NormalToPointerOver" position.
5. Check if the provided IRichAnimatedIconSource's markers for the marker "PointerOver". If it is found, then we will hard cut to the "PointerOver" position.
6. Check if the provided IRichAnimatedIconSource's markers has any marker which ends with "ToPointerOver_End". If any marker is found which has that ending, we will hard cut to the first marker found with the appropriate ending's position.
7. Check if "PointerOver" parses to a float. If it does, animated from the current position to the parsed float. **
8. Hard cut to position 0.0.

## Generalization
Here is the same algorithm but with variable names [PreviousState] and [NewState].

1. Check the provided IRichAnimatedIconSource's markers for the markers "[PreviousState]To[NewState]_Start" and "[PreviousState]To[NewState]_End". If both are found play the animation from "[PreviousState]To[NewState]_Start" to "[PreviousState]To[NewState]_End".
2. If "[PreviousState]To[NewState]_Start" is not found but "[PreviousState]To[NewState]_End" is found, then we will hard cut to the "[PreviousState]To[NewState]_End" position.
3. If "[PreviousState]To[NewState]_End" is not found but "[PreviousState]To[NewState]_Start" is found, then we will hard cut to the "[PreviousState]To[NewState]_Start" position.
4. Check if the provided IRichAnimatedIconSource's markers for the marker "[PreviousState]To[NewState]". If it is found then we will hard cut to the "[PreviousState]To[NewState]" position.
5. Check if the provided IRichAnimatedIconSource's markers for the marker "[NewState]". If it is found, then we will hard cut to the "[NewState]" position.
6. Check if the provided IRichAnimatedIconSource's markers has any marker which ends with "To[NewState]_End". If any marker is found which has that ending, we will hard cut to the first marker found with the appropriate ending's position.
7. Check if "[NewState]" parses to a float. If it does, animated from the current position to the parsed float. **
8. Hard cut to position 0.0.

## Slider Scenario**
We want Animated Icon to be usable with sliders as well, where the slider's value drives the icon. I think there are two ways to do this, we can add step 7 to the segment lookup described above, which allows you to tie the sliders value to the animated Icon's state property with a binding (and converter).  The other option is to tell consumers to wrap the lottie gen output in their own implementation of IRichAnimatedIconSource which overrides the Marker property's Lookup() method to return a proper segment when float is asked for. We've decided to implement the former since it is much simplier for the consumer, albeit a bit magical and difficult to discover.

