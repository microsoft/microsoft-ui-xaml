AnimatedVisualPlayer Spec
===

# Background

Today, developers can create animated visuals to use in their UI by using the
[AnimatedVisualPlayer](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer)
control.
`AnimatedVisualPlayer` can play any
[IAnimatedVisual](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.IAnimatedVisual)
implementation,
but the typical way to get an `IAnimatedVisual` implementation is to use
the LottieGen tool to generate code from a JSON file that was created from Adobe AfterEffects.

[More information on Lottie and AnimatedVisualPlayer](https://docs.microsoft.com/en-us/windows/communitytoolkit/animations/lottie)

There are some performance issues with the current implementation of LottieGen and AnimatedVisualPlayer: 
1)	Complex Lottie animations can instantiate a lot of composition animations 
which may impact performance and overall battery life. 
2)	Lottie files that have a lot of complex animations
have some cost to performance even when they are paused. 
The Idle (paused) Lottie animations use the same amount of CPU resources as
when they are playing the actual animation.

The new APIs here allow `AnimatedVisualPlayer`,
[AnimatedIcon](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.AnimatedIcon),
and the interface generated by LottieGen to be updated to help reduce the number of composition animations created
when the animation is idle or paused to help improve performance when a developer uses
Lottie Animations with `AnimatedVisualPlayer`.

This spec will focus on changes for LottieGen and `AnimatedVisualPlayer`. 

This will require 3 changes: 
1.	Create a new interface for which an implementation is generated by LottieGen called `IAnimatedVisual2`,
that can destroy and instantiate animations.
2.	Create a new interface `IAnimatedVisualSource3` from which to get the new `IAnimatedvisual2`
3.	Add a `AnimationOptimization` property to AnimatedVisualPlayer that will control
when animations should be destroyed and when they should be instantiated.
This will be implemented using the previous new interfaces.

# API Pages

## AnimatedVisualPlayer.AnimationOptimization property

Gets or sets a value that determines how animations are cached. Defaults to 'Latency'.

The two modes for the property will specify the behavior the AnimatedVisualPlayer will have
when the player is idle (when `PlayAsync` is not active).

|Mode| Behavior|
|---- | -------|
|Latency| AnimatedVisualPlayer will pre-create animations even before `PlayAsync()` is called,
and not destroy any when player is idle. 
|Resources | AnimatedVisualPlayer will not create animations until `PlayAsync()` is called,
and will destroy them when it completes. If you call pause, this does not free up all 
resources of the player. To truly stop the animation, call Stop. 

Note: If you set the `Source` or `AnimationOptimization` of your player, the player will defer 
processing of the source file until the layout is being formed.

If you have an animation that will start on click, you may want to initialize 
the property to `None` and set `AnimationOptimization` to 'Always'
when the mouse enters this control, 
and back to `None` when mouse leaves to ensure that the animation starts immediately on click,
but don't use up resources otherwise.

Setting the `AnimationOptimization` to `Latency` when the mouse enters the control will ensure that
the animation objects are preloaded so that the animation may start immediately when it needs to play. 

If you call PlayAsync before animations are loaded, then the player won't start until all animations are loaded completely.

```c#
private void Player_PointerEntered(object sender, PointerRoutedEventArgs e)
{
    myanimatedvisualplayer.AnimationOptimization = AnimationOptimization.Latency;
}
async private void Player_PointerExited(object sender, PointerRoutedEventArgs e)
{              
    myanimatedvisualplayer.AnimationOptimization = AnimationOptimization.Resources;
}

async private void Player_OnClick(object sender, PointerRoutedEventArgs e)
{              
    myanimatedvisualplayer.PlayAsync(0.0, 1.0);
}

```

If `PlayAsync` is called and the animations have not been instantiated due to `AnimationOptimization` being set to `None`,
`PlayAsync` will instantiate the animations so it can play the visual. 

# API Details

```c#
namespace Microsoft.UI.Xaml.Controls
{
  unsealed runtimeclass AnimatedVisualPlayer 
  {
      // Existing ...

    //Sets the caching behavior of the AnimatedVisualPlayer
    AnimationOptimization AnimationOptimization;
  }

  public enum AnimationsCacheMode
  {
      Always = 0,
      None
  }
}
```

## IAnimatedVisual2 Interface

An extension to
[IAnimatedVisual](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.IAnimatedVisual)
that adds the ability to control when animation objects are created and destroyed.

| Method | Meaning |
|-------------|---------|
| CreateAnimations | Instantiates the animation visuals for the provided progress value of the animation. |
| DestroyAnimations | Destroys all animation visuals for a given animation.|

```c# 
interface IAnimatedVisual2 requires IAnimatedVisual
{
    void InstantiateAnimations(Double progressHint);
    void DestroyAnimations();
};
```

## IAnimatedVisual2.InstantiateAnimations Method

Instantiates the animation visuals for the provided progress value of the animation. 

```c# 
void CreateAnimations();
```

## IAnimatedVisual2.DestroyAnimations Method

Destroys all animation visuals for a given animation. 

```c# 
void DestroyAnimations();
```
## IAnimatedVisualSource3 Interface

An extension to
[IAnimatedVisualSource](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.IAnimatedVisualSource)
that products an `IAnimatedVisualSource3`.

```c# 
interface IAnimatedVisualSource3
{
    IAnimatedVisual2 TryCreateAnimatedVisual(
        Windows.UI.Composition.Compositor compositor,
        out Object diagnostics,
        Boolean instantiateAnimations,);
};

```
