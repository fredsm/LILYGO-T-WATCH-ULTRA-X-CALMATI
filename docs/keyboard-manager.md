# Shared watch keyboard

The watch has a 410 × 502 portrait framebuffer, but the physical AMOLED
corners are rounded. Full-width keyboards anchored at the bottom used to
place their edge keys under the bezel.

All LVGL text/numeric keyboard instances now use `keyboard_manager.h`.
The individual screens still own their keyboard widgets and callbacks, so
a Wi-Fi password submission, message send or numeric field commit retains
its original behavior.

- `watch_keyboard_create(parent, profile)`: creates a hidden keyboard with
  shared geometry, compact key spacing and consistent legible font.
- `watch_keyboard_show(keyboard, textarea)`: binds the existing input field,
  brings the keyboard forward and temporarily moves/resizes the input field
  when it would be clipped or covered. It retains the textarea's password
  mode, maximum length and allowed characters.
- `watch_keyboard_hide(keyboard)`: hides the keyboard, detaches the field
  and restores its original size, alignment and stacking order.
- Screen unload restores any temporarily moved field. Deleting a field or
  keyboard also clears the associated manager session.

Profiles share a 350-pixel width (30-pixel side margins) and 44-pixel bottom
inset. Standard height is 220 pixels, compact 200, and compose 300. LVGL's
native lowercase, uppercase, symbols and number modes are retained. The
separate lock-screen PIN keypad remains unchanged.

The desktop simulator masks the rounded corners and rejects new touch
contacts there. The radius (90 logical pixels) is an approximation, not a
measurement of the real AMOLED. Confirm margins on hardware before
tightening them.

The `rounded_keyboard_geometry` regression test opens all 11 original
keyboard instances, checks every actual key rectangle against the mask,
checks active field visibility, and taps edge keys, modes, backspace,
confirmation and cancellation. It also verifies that low input fields
return to their original positions after editing or screen navigation.
