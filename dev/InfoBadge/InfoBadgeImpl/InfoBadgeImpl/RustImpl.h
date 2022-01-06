#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct String;

struct CornerRadius {
  double TopLeft;
  double TopRight;
  double BottomLeft;
  double BottomRight;
};

extern "C" {

CornerRadius get_fully_rounded_corner_radius_value_rust_impl(double height);

void animated_icon_transition_states(String fromState, String toState, float playbackMultiplier);

} // extern "C"
