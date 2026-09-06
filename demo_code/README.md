# P4-B Demo Code

Reference material only, and more than usually worth trying yourself first: this project's
whole point is understanding the HID report descriptor, not having a working binary. Design
your own descriptor and try enumeration yourself before opening this.

`01_gamepad_reference/` is a complete, working USB HID gamepad, built by starting from NXP's
own verified `usb_device_hid_mouse` example (in `SDK_2_2_0_FRDM-KL26Z/boards/frdmkl26z/
usb_examples/`) and changing only what a gamepad actually needs to differ from a mouse:

- `usb_device_ch9.c/.h`, `usb_device_class.c/.h`, `usb_device_hid.c/.h`, `usb_device_config.h`:
  copied verbatim, unmodified. These implement the generic USB enumeration state machine and
  HID class request handling; none of it is mouse-specific or needs to change for a gamepad.
- `board.c/.h`, `clock_config.c/.h`: copied verbatim. `clock_config.c` is byte-for-byte
  identical (confirmed by hash) to the one every other project in this series has been reusing,
  and it configures the crystal-referenced PLL clock mode USB actually requires, so this
  series' whole clock setup was USB-compatible from day one without anyone needing to know it.
- `usb_device_descriptor.c/.h`: rewritten. New HID report descriptor (2 absolute axes, 2
  buttons, see the manual, Section 8, for the byte-by-byte design and a verification script),
  corrected SubClass/Protocol (0x00/0x00, not the mouse's boot-protocol values), a distinct
  Product ID, and new strings.
- `gamepad.c/.h` (renamed from `mouse.c/.h`): rewritten. Reads the accelerometer (I2C0),
  the push button, and the touch slider, all reused verbatim from P1, and builds the report
  from live sensor data instead of the mouse example's bouncing-square animation.
- `pin_mux.c`: extended with I2C0, SW1, and the TSI electrode pins (P1's pins, same values).
  No USB pins are configured anywhere, on purpose: the USB D+/D- lines are dedicated, not part
  of the general PORT mux system, confirmed by their absence in NXP's own example too.

**This project was compiled and verified at the byte level, not just written.** It builds
cleanly against `SDK_2_2_0_FRDM-KL26Z` (36 KB of the 128 KB flash budget). The HID report
descriptor was designed with a Python script that counts each item's exact byte length (see
the manual, Section 8), and after compiling, the actual bytes were extracted straight out of
the built ELF file and checked against that design; they matched exactly, byte for byte. It has
not been flashed to physical hardware, and USB enumeration specifically is not the kind of
thing that can be verified any other way; a project leader must plug a real board into a real
PC before WS8 and check it actually enumerates as a driver-free gamepad. One pre-existing
compiler warning (a `sizeof` divisor using the wrong struct type on line ~78 of
`usb_device_descriptor.c`) was inherited unchanged from NXP's own original mouse example, not
introduced here; it's harmless for a single-interface device but worth knowing about if you go
looking for warnings.

To build it, see the P0 manual, Section 8, for the general `cmake` / `make` / `objcopy` flow;
`armgcc/` here already has its build scripts patched for this repository's folder layout, the
same fixes every prior project's combined reference has needed, applied to a real NXP example's
own build files this time rather than one written from scratch for this series.
