#[repr(C)]
pub struct CornerRadius {
    TopLeft: f64,
    TopRight: f64,
    BottomLeft: f64,
    BottomRight: f64,
}

#[no_mangle]
pub extern fn get_fully_rounded_corner_radius_value_rust_impl(height: f64) -> CornerRadius {
    let height = height / 2.0;
    CornerRadius{TopLeft: height, TopRight: height, BottomLeft: height, BottomRight: height}
}


#[no_mangle]
pub extern fn animated_icon_transition_states(fromState: String, toState: String, playbackMultiplier: f32) {

}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn get_fully_rounded_corner_radius_value_rust_impl_works() {
        assert_eq!(get_fully_rounded_corner_radius_value_rust_impl(2.0), (1.0,1.0,1.0,1.0));
        assert_eq!(get_fully_rounded_corner_radius_value_rust_impl(3.0), (1.5,1.5,1.5,1.5));
    }
}
