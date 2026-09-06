# P4-B: Reference

*Part of the P4-B manual split. See `P4B_1_Start_Here.md` for the full file list and how to use this manual.*

---

## 13. Code Structure Explanation

| File | What It Does |
|---|---|
| `usb_device_ch9.c/.h` | Generic USB chapter-9 enumeration state machine. Unmodified from NXP's mouse example. |
| `usb_device_class.c/.h`, `usb_device_hid.c/.h` | Generic HID class request handling. Unmodified. |
| `usb_device_descriptor.c/.h` | Device, configuration, and report descriptors (Sections 7, 8, 11); VID/PID and strings (Section 20). The only descriptor-level file that changed from the mouse example. |
| `gamepad.c/.h` (renamed from `mouse.c/.h`) | `InitGamepadSensors()` (I2C0, SW1, TSI0, all reused from P1), `ReadAccelAxisMg()`, `ScaleMgToAxis()`, `ReadTouchButton()`, and `USB_DeviceHidGamepadAction()` (Section 9), which replaces the mouse example's animated bouncing-square logic with real sensor reads. |
| `pin_mux.c` | UART0 console plus I2C0, SW1, and the TSI electrode, all pins reused verbatim from P1. |

## 14. Session Plan (maps to Guideline Section 4.6)

| Meeting | Phase | What Members Do | Deliverable | Slide Focus |
|---|---|---|---|---|
| 1 of 2 | USB stack setup and descriptor | Configure the USB clock and stack (Section 6). Write the HID report descriptor, axes and buttons (Section 8). Verify enumeration: the device appears in Device Manager / System Information as a HID device, no driver error. | Board enumerates as a HID device. No driver error in Device Manager. Descriptor accepted by the OS. | Screenshot of Device Manager / System Information showing clean enumeration; what the group's own descriptor design looked like before comparing against the reference. |
| Independent (before Nov 19) | Report loop, sensor mapping, and demo prep | Read the accelerometer and touch slider in the report-generation path (Section 9). Format into the report struct. Send at the 10 ms interval (Section 10). Validate all axes and buttons on a Gamepad API tester. | All axes and buttons responding in the browser gamepad tester. Smooth axis movement from board tilt. | Live demo is the whole slide; be ready to trace through the report descriptor field by field on request. |

## 15. Milestones and Success Criteria

| Milestone | Success Criteria | Evidence |
|---|---|---|
| Report descriptor designed and byte-counted | Group can state the report descriptor's total length and explain what determines it | Verbal check by leader |
| Clean enumeration (Session 1 exit criteria) | Device Manager / System Information shows a HID device, no driver error, no "unknown device" | Screenshot |
| Gamepad API recognizes the device | `gamepadconnected` fires in a browser console | Live demo |
| Axes respond to tilt | Both X and Y move smoothly and saturate correctly at extreme tilt (not wrapping to a nonsense value) | Live demo |
| Buttons respond | SW1 and the touch slider both independently toggle their mapped buttons | Live demo |
| Full pipeline (final exit criteria) | Board plugs into any PC USB port, enumerates with no driver install, and a browser gamepad tester shows both axes and both buttons responding live | Live demo |

## 16. Sample Output

Device Manager (Windows), after a successful enumeration: a "HID-compliant game controller" entry under Human Interface Devices, with a Details tab showing Hardware Ids referencing `VID_1FC9&PID_0092`.

Browser console, after plugging in:

```
GamepadEvent { gamepad: Gamepad { id: "TXST IEEE GAMEPAD (Vendor: 1fc9 Product: 0092)", ... } }
```

Tilting the board right should move axis 0 toward +1 (scaled from the raw -127..127 report value); pressing SW1 should flip `buttons[0].pressed` to `true`.

## 17. Project-Specific Debugging Reference

| # | Common Problem | Suggested Debugging Steps | Difficulty |
|---|---|---|---|
| 1 | Device doesn't appear at all, not even as an unknown device | Confirm the correct USB connector was used (target USB, not the SDA programming port), and that `USB_DeviceRun()` is actually being reached; add a temporary `PRINTF` right before it. | Easy |
| 2 | Device Manager shows a driver error or "unknown device" | Almost always a malformed configuration or HID descriptor: recheck `USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL` and `USB_DESCRIPTOR_LENGTH_HID_GAMEPAD_REPORT` exactly match the real byte counts of their respective arrays; a length field that's even one byte off from the actual data will confuse the enumeration parser. | Hard |
| 3 | Device enumerates fine, but the browser's Gamepad API never sees it | Check the top-level `Usage Page`/`Usage` are exactly `Generic Desktop`/`Gamepad` (`0x05,0x01` then `0x09,0x05`); some OS-level HID layers are more permissive about unusual top-level usages than the Gamepad API implementations browsers ship. | Hard |
| 4 | Axes report but never move, or are stuck at one extreme | Confirm `InitGamepadSensors()` actually found the accelerometer (its I2C probe can fail silently on a wiring issue); add a temporary debug print of `g_accelAddr` after init, `0` means it was never found. | Medium |
| 5 | Axes move but feel "backward" or saturate too early/late | Check the sign convention: `ScaleMgToAxis()`'s clamp range (±1000 mg to ±127) is a design choice, not a derived fact; a group is free to widen or narrow it, but should do so deliberately and explain why during the demo. | Easy |
| 6 | Buttons never register | For SW1: confirm the active-low read (`== 0` means pressed). For the touch slider: confirm `TSI_Calibrate()` ran with nothing touching the electrode; a bad baseline makes every reading look like "always touched" or "never touched." | Medium |
| 7 | Everything works, but only when the debugger/IDE is attached, not standalone | Check for a leftover blocking wait on something that only resolves when a debugger is present (a common mistake is leaving a `while` loop waiting on a condition that a debug probe happens to satisfy); nothing in this reference should behave this way, but it's worth ruling out first if "works in the IDE, not standalone" comes up. | Hard |

## 18. Glossary

- **USB enumeration:** the request-response sequence a host and device go through after connection, before the device is usable.
- **Descriptor:** a structured block of bytes a USB device provides describing itself; layered from device, to configuration, to interface, to class-specific descriptors like HID's report descriptor.
- **HID (Human Interface Device):** a USB device class with a standardized, self-describing report format, letting the OS's built-in generic HID driver handle any compliant device without a device-specific driver.
- **HID report descriptor:** the compact binary language describing exactly what a HID device's reports contain, bit by bit.
- **Endpoint:** a numbered, directional data channel on a USB device; endpoint 0 is always the control endpoint used for enumeration itself.
- **Interrupt transfer (USB):** a transfer type where the host polls the device at a guaranteed maximum interval; not related to CPU interrupts despite the shared name.
- **Boot protocol:** a simplified, fixed HID report format the spec defines only for keyboards and mice, so a PC's BIOS can use them before a full driver loads; not applicable to a gamepad.
- **Gamepad API:** a browser JavaScript API (`navigator.getGamepads()`) that exposes connected HID gamepads directly to web pages, used here as the validation tool.

## 19. References

- `SDK_2_2_0_FRDM-KL26Z/boards/frdmkl26z/usb_examples/usb_device_hid_mouse/bm/`: the verified NXP example this project starts from.
- `middleware/usb_1.6.3/` (in the SDK): the generic USB device stack (KHCI controller driver, chapter-9 state machine, HID class handling) this project reuses unmodified.
- USB HID Usage Tables (usb.org, free): the registry of standard Usage Page and Usage values, source for `Generic Desktop`/`Gamepad` (`0x05,0x01`/`0x09,0x05`) and the button usage page.
- Device Class Definition for HID 1.11 (usb.org, free): the specification defining report descriptor syntax, SubClass/Protocol semantics, and the boot protocol.
- P1's manual (`P1_Sensor_Dashboard/`, this repository): the accelerometer, SW1, and touch slider setup this project reuses without re-deriving.

## 20. Developer Notes

- **`demo_code/01_gamepad_reference/` was compiled and verified at the byte level, not just written.** It builds cleanly (36 KB of 128 KB flash) with no warnings introduced by this project's own changes. The report descriptor's actual compiled bytes were extracted from the built ELF and checked against the intended 50-byte design; they matched exactly. It has not been flashed to physical hardware or plugged into a real PC. Unlike every prior project's "bench-test before the session" note, this one carries extra weight: USB enumeration genuinely cannot be verified any way other than plugging a real device into a real host, so a leader's real-hardware check before WS8 is not optional the way it might feel for some earlier projects.
- **PID choice:** `0x0092` was picked arbitrarily, one more than the stock mouse example's `0x0091`, purely so the two don't collide if both happen to be plugged in at once. The VID (`0x1FC9`) is NXP's real, USB-IF-assigned Vendor ID, reused from NXP's own example; this is standard, low-risk practice for a non-commercial educational device, not something a real shipped product could do (a real product needs its own purchased VID).
- **One pre-existing compiler warning** (a `sizeof` divisor using `usb_device_interfaces_struct_t` instead of `usb_device_interface_struct_t` around line 78 of `usb_device_descriptor.c`) was inherited unchanged from NXP's own original mouse example. It's harmless for a single-interface device like this one; it wasn't introduced by this project's changes and wasn't "fixed" either, to keep the diff against the verified original as small and auditable as possible.
- If a future project wants a richer report (more axes, a D-pad, more buttons), the extension point is entirely Section 8's descriptor plus the corresponding fields in `USB_DeviceHidGamepadAction()`; nothing about the generic USB stack files needs to change.

---
*IEEE Texas State University Student Branch. Connect. Build. Inspire.*
