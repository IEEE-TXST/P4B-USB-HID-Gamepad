# TXST IEEE Student Branch: FRDM-KL26Z Project Series
## Capstone Manual, P4-B: USB HID Gamepad

**Document status:** DRAFT v0.1, for project-leader review and bench testing before member use
**Track:** Embedded Systems | **Difficulty:** Hard | **Sessions:** 1 (WS8, Nov 5, 2026), plus independent build before the Nov 19 demo
**Groups:** 2 groups of 3, working independently
**Companion documents:** P0's manual (`P0_Board_Orientation_and_Toolchain_Setup/`, start at `P0_1_Start_Here.md`) through `P3_Project_Manual.md` (read all four first), *TXST IEEE FRDM-KL26Z Project Specification*
**Hardware:** FRDM-KL26Z only (uses the on-board full-speed USB controller)
**Demo code:** see `demo_code/` in this project's folder

---

## How to Use This Manual

Same rule as every prior manual, with the same extra emphasis P4-A gave the whiteboard step: this project's entire point is understanding the HID report descriptor well enough to have designed one yourself, not having a working binary. Try designing your own descriptor and watching enumeration fail in informative ways before opening `demo_code/`.

Project leaders: bench-test `demo_code/01_gamepad_reference/` end to end, on a real board plugged into a real PC, before WS8. USB enumeration is uniquely unforgiving to debug blind: when something is wrong, the usual symptom is not an error message, it's the device simply not showing up, or Windows silently falling back to a generic (and wrong) driver. A leader who has seen this exact firmware enumerate correctly once has something concrete to compare a stuck group's board against.

**A note on accuracy:** this project was built by starting from NXP's own shipped, working USB HID mouse example, not by writing USB stack code from scratch, because the generic USB enumeration and HID class-request handling in that example needs zero changes to work for any HID device, gamepad included. What actually changed (the report descriptor, the SubClass/Protocol codes, the sensor-to-report mapping) is documented field by field in this manual, and the compiled binary's actual descriptor bytes were extracted and checked against the design after building, not just assumed correct. Section 21 has the full trail.

---

## 0. Why This Session Exists

Every device you've plugged into a computer, a mouse, a keyboard, a flash drive, went through a conversation with the operating system before it did anything useful: "here's what kind of device I am, here's what I can do, here's how my data is structured." That conversation is USB enumeration, and for the specific case of HID (Human Interface Devices), it's also why a mouse or a keyboard just works the instant you plug it in, no driver install, no restart. P4-B is where you make the FRDM-KL26Z hold up its end of that conversation convincingly enough that a completely unmodified PC believes it's a real gamepad. This is a genuinely different kind of hard than the earlier capstone: there's no compiler error waiting for you if you get a descriptor field wrong, only a device that mysteriously doesn't work, or works partially, or works on one OS and not another. Patience and careful, byte-by-byte verification, the habit this whole manual series has tried to model, matter more here than almost anywhere else in the curriculum.

## 1. New Concepts You Need Before Starting

Builds on P1 (the accelerometer, touch slider, and interrupts this project reuses directly) and P2/P3 (both complete, per the prerequisite, for general comfort with peripheral configuration and interrupt-driven firmware). Everything below is new to P4-B.

**USB enumeration.** The sequence a USB device and host go through the instant a device is plugged in: the host resets the device, asks for its device descriptor (a small summary: vendor, product, device class), assigns it a unique address on the bus, asks for its configuration descriptor (a longer structure describing interfaces and endpoints), and finally activates that configuration. Only after all of this succeeds does the device actually start working. Every step is a request-response exchange on endpoint 0 (the "control endpoint"), which every USB device has by definition.

**Descriptors, and why they're layered.** A device descriptor is small and generic (this device exists, here's broadly what it is). A configuration descriptor is bigger and contains one or more interface descriptors (what distinct functions this device offers). An interface can further contain class-specific descriptors, for a HID device, that means a HID descriptor plus a report descriptor. This layering is what lets a single USB device present as several distinct pieces of functionality if needed (a webcam with a microphone, for example, is two interfaces in one device); this project only ever has one interface, but the same layered structure is still there.

**The HID report descriptor.** A separate, compact binary language (not free text, not readable without a decoder) that tells the host, precisely, byte and bit, what a HID device's reports contain: how many bytes, which bits mean what, and their valid range. Windows, macOS, Linux, and every browser all include a generic HID report descriptor parser built into the OS itself, which is the entire reason HID devices need no driver: the OS doesn't need to know in advance what a "gamepad" or "custom sensor device" looks like, it reads the description the device itself provides at enumeration time and configures itself on the spot. This is also why getting the descriptor wrong is a uniquely quiet kind of bug: the OS will happily accept and try to use a malformed descriptor, just incorrectly.

**HID vs. CDC, and why one needs a driver and the other doesn't.** CDC (Communication Device Class, what a virtual serial port uses, like this board's own OpenSDA connection) has a fixed, simple report format the class spec itself defines, but interacting with it as "a serial port" requires OS-specific driver glue to expose it that way to applications. HID's self-describing report descriptor means the OS can build a generic, correct interface to the device without ever needing device-specific code: this is a deliberate design trade-off in the USB HID specification, not an accident, and it's exactly why this project's descriptor design work is the thing that makes driver-free operation possible at all.

**Endpoints.** A USB device's control endpoint (endpoint 0) handles descriptor requests and configuration; separate from that, most devices have one or more additional endpoints for their actual data. This project uses one interrupt IN endpoint: "interrupt" here doesn't mean a CPU interrupt, it's a USB transfer type meaning the host polls this endpoint at a fixed interval (Section 10) and the device replies with its latest data whenever asked.

**Sensor-to-report mapping.** The board's actual sensors (14-bit accelerometer readings in milli-g, a digital button state, a touch-slider threshold) don't naturally look like a HID gamepad report (signed 8-bit axes, single-bit buttons). Converting from one representation to the other, correctly, with sensible clamping at the edges, is genuine engineering work, covered in Section 9.

## 2. Purpose

By the end of this project, every group has: a HID report descriptor they understand well enough to explain field by field; a board that enumerates on any PC as a driver-free gamepad; two absolute axes driven by real accelerometer tilt; two buttons driven by the push button and the touch slider; and a live demonstration on a browser-based Gamepad API tester at the Nov 19 showcase.

## 3. Prerequisites

P1 (I2C accelerometer, touch slider, interrupts), P2, and P3 all complete. This project reuses P1's exact, already-verified accelerometer I2C sequence, SW1 button setup, and TSI touch slider code without re-deriving any of it; if those feel shaky, revisit `P1_Project_Manual.md` first.

## 4. Additional Toolchain for This Project

Everything from prior projects still applies. New for P4-B: **any modern browser** (Chrome, Firefox, Edge) to validate enumeration via its built-in Gamepad API, no install needed; a page like `gamepad-tester.com`, or a small local HTML page calling `navigator.getGamepads()`, both work. **USBPcap and Wireshark** are optional, for groups who want to actually watch the enumeration traffic and see the report descriptor bytes as the OS parses them; not required for the exit criteria, but genuinely illuminating if a group gets stuck and wants to see exactly what the host is asking for and how the board answers.

## 5. Hardware and Software Reference

| Fact | Value | Verified against |
|---|---|---|
| USB controller type on this chip | KHCI (Kinetis's full-speed device controller), driven by `middleware/usb_1.6.3/device/usb_device_khci.c` | `usb_device_config.h`'s `USB_DEVICE_CONFIG_KHCI` setting; confirmed working in the mouse example |
| USB clock requirement | Full-speed USB needs an accurate 48 MHz clock; this project's `clock_config.c` is byte-for-byte identical (confirmed by file hash) to the one every prior project in this series has used, and it runs the chip in PEE mode (crystal-referenced PLL) | `clock_config.c` diff/hash comparison against `driver_examples/pit/clock_config.c`; `mcgMode: kMCG_ModePEE` confirmed in the file |
| USB pins | None configured anywhere; USB D+/D- are dedicated pins, not part of the general PORT mux system | Confirmed by their total absence from the mouse example's own `pin_mux.c`, and from this project's |
| VID | `0x1FC9`, NXP's real, USB-IF-assigned Vendor ID | Reused from NXP's own mouse example's device descriptor bytes |
| PID | `0x0092` (arbitrary, distinct from the stock mouse example's `0x0091`) | This project's own choice; see Section 20 |
| Report descriptor length | 50 bytes | Counted by a Python script (Section 8) and independently confirmed by extracting the actual compiled bytes from the built ELF file |
| Endpoint polling interval | 10 ms (full-speed `bInterval` is direct milliseconds, unlike high-speed's power-of-two encoding) | `usb_device_descriptor.h`, this project's own value, chosen per the spec's session plan |

## 6. Reusing NXP's Own USB Stack, Deliberately

`demo_code/01_gamepad_reference/` is not written from scratch. It starts from `SDK_2_2_0_FRDM-KL26Z/boards/frdmkl26z/usb_examples/usb_device_hid_mouse/bm/`, NXP's own shipped, working example, and changes only what a gamepad genuinely needs to differ from a mouse. This is a deliberate choice, not a shortcut: `usb_device_ch9.c` (the chapter-9 enumeration state machine every USB device needs) and `usb_device_class.c`/`usb_device_hid.c` (generic HID class request handling) contain zero mouse-specific code, confirmed by checking, and reimplementing several hundred lines of USB protocol state machine by hand, for a part of the system that doesn't need to change, would only introduce risk without teaching anything the mouse example doesn't already demonstrate correctly. What you should spend your own effort understanding and being able to explain is exactly the part that *did* change: Sections 8 through 10.

## 7. USB Enumeration, Traced Through This Project

When the board is plugged in, here is what actually happens, in order, before a single sensor reading ever reaches the host:

1. The host resets the bus and requests the **device descriptor** (`g_UsbDeviceDescriptor` in `usb_device_descriptor.c`): VID, PID, device class (0x00, meaning "look at the interface descriptors to find out," standard for HID devices, which declare their class at the interface level instead).
2. The host requests the **configuration descriptor** (`g_UsbDeviceConfigurationDescriptor`), which in one contiguous block contains the configuration descriptor itself, one interface descriptor (class 0x03 = HID, SubClass 0x00, Protocol 0x00, see Section 8's callout on why those last two are 0x00 and not the mouse's boot-protocol values), one HID descriptor (which itself just says "and here's how long my report descriptor is"), and one endpoint descriptor (the interrupt IN endpoint, Section 10).
3. The host separately requests the **HID report descriptor** itself (Section 8), the actual byte-for-byte definition of what a report from this device means.
4. The host activates the configuration, and from this point on, the device is "configured": `USB_DeviceHidGamepadCallback` (Section 9) starts actually sending reports.
5. The OS's own built-in HID parser reads the report descriptor once, at this point, and from then on knows exactly how to interpret every report this device sends, with zero device-specific code of its own.

Every one of these is a real, observable USB transaction; if a group has Wireshark and USBPcap set up, this entire sequence is visible, request by request, which is the fastest way to see exactly where an enumeration is failing if it does.

## 8. Designing the HID Report Descriptor

This is the actual center of the project. `g_UsbDeviceHidGamepadReportDescriptor` in `usb_device_descriptor.c`:

```c
uint8_t g_UsbDeviceHidGamepadReportDescriptor[USB_DESCRIPTOR_LENGTH_HID_GAMEPAD_REPORT] = {
    0x05U, 0x01U, /* Usage Page (Generic Desktop) */
    0x09U, 0x05U, /* Usage (Gamepad) */
    0xA1U, 0x01U, /* Collection (Application) */

    0x09U, 0x01U, /*   Usage (Pointer) */
    0xA1U, 0x00U, /*   Collection (Physical) */
    0x05U, 0x01U, /*     Usage Page (Generic Desktop) */
    0x09U, 0x30U, /*     Usage (X) */
    0x09U, 0x31U, /*     Usage (Y) */
    0x15U, 0x81U, /*     Logical Minimum (-127) */
    0x25U, 0x7FU, /*     Logical Maximum (127) */
    0x75U, 0x08U, /*     Report Size (8) */
    0x95U, 0x02U, /*     Report Count (2): X, then Y */
    0x81U, 0x02U, /*     Input (Data, Variable, Absolute) */
    0xC0U,        /*   End Collection (Physical) */

    0x05U, 0x09U, /*   Usage Page (Button) */
    0x19U, 0x01U, /*   Usage Minimum (Button 1) */
    0x29U, 0x02U, /*   Usage Maximum (Button 2) */
    0x15U, 0x00U, /*   Logical Minimum (0) */
    0x25U, 0x01U, /*   Logical Maximum (1) */
    0x95U, 0x02U, /*   Report Count (2) */
    0x75U, 0x01U, /*   Report Size (1) */
    0x81U, 0x02U, /*   Input (Data, Variable, Absolute) */
    0x95U, 0x01U, /*   Report Count (1) */
    0x75U, 0x06U, /*   Report Size (6): padding */
    0x81U, 0x01U, /*   Input (Constant) */

    0xC0U /* End Collection (Application) */
};
```

**What each item means, field by field:**

- **Usage Page / Usage** identify, from a standard registry every OS already knows, what kind of thing this is. `Usage Page (Generic Desktop)` plus `Usage (Gamepad)` tells the OS "treat this the way you treat any gamepad," which is what makes it show up correctly in a browser's Gamepad API or a game's controller list without any custom code on the host side.
- **Collection (Application)** ... **End Collection** wraps the entire report in one logical unit; a **Collection (Physical)** nested inside groups the two axes together as one physical control (a stick), which is the conventional way HID gamepad descriptors structure axis data.
- **Logical Minimum / Logical Maximum** define the actual numeric range a field can hold: `-127` to `127` for the axes, matching a signed 8-bit range (deliberately avoiding `-128`, which doesn't have a clean two's-complement counterpart in this range and which some HID parsers handle inconsistently).
- **Report Size / Report Count** define, respectively, how many bits one field occupies and how many consecutive fields of that size follow. `Report Size (8), Report Count (2)` means "two 8-bit fields," X then Y.
- **Input (Data, Variable, Absolute)** is the actual declaration that this range of bits is real, changing data (`Data`, not `Constant`), one independent value per field (`Variable`, not `Array`), and a real position rather than a delta since the last report (`Absolute`, not `Relative`, the opposite of what the mouse example used for its relative mouse-motion axes).
- **The button block** works the same way, but with `Usage Page (Button)` and a `Usage Minimum`/`Usage Maximum` range instead of named X/Y usages, the standard HID convention for a block of generic buttons.
- **The final `Report Count (1), Report Size (6), Input (Constant)`** is padding: 2 buttons only need 2 bits, but HID reports are conventionally byte-aligned, so the remaining 6 bits of that byte are declared as constant, meaningless padding rather than left ambiguous.

**Report layout this produces**, 3 bytes total, no report ID:

| Byte | Content |
|---|---|
| 0 | X axis, signed 8-bit, -127 to 127 |
| 1 | Y axis, signed 8-bit, -127 to 127 |
| 2 | bit 0 = button 1, bit 1 = button 2, bits 2-7 = padding (always 0) |

**This descriptor was verified two ways, not just written and trusted.** First, a small Python script (below) computed its exact length by summing each item's byte size, confirming 50 bytes before the array was even finalized. Second, after compiling, the actual bytes were extracted directly from the built ELF file (`arm-none-eabi-objdump`, reading the `.data` section at the symbol's address) and compared against the intended array; they matched exactly, byte for byte, confirming nothing was mistyped or miscounted between design and compiled binary.

```python
desc = [
    0x05,0x01, 0x09,0x05, 0xA1,0x01,
      0x09,0x01, 0xA1,0x00, 0x05,0x01, 0x09,0x30, 0x09,0x31,
      0x15,0x81, 0x25,0x7F, 0x75,0x08, 0x95,0x02, 0x81,0x02, 0xC0,
      0x05,0x09, 0x19,0x01, 0x29,0x02, 0x15,0x00, 0x25,0x01,
      0x95,0x02, 0x75,0x01, 0x81,0x02, 0x95,0x01, 0x75,0x06, 0x81,0x01,
    0xC0,
]
print(len(desc))  # 50
```

## 9. Sensor-to-Report Mapping

`gamepad.c`'s `USB_DeviceHidGamepadAction()` is called once to kick off the very first report, and then again automatically every time the previous report finishes sending (the `kUSB_DeviceHidEventSendResponse` event in `USB_DeviceHidGamepadCallback`), which means it needs no timer of its own; the natural pacing comes from the host polling the endpoint every 10 ms and the stack asking for a fresh report each time one is consumed.

Each call reads live sensor data and builds the 3-byte report:

```c
int16_t xMg = ReadAccelAxisMg(0x01U); /* OUT_X_MSB, same register P1 used */
int16_t yMg = ReadAccelAxisMg(0x03U); /* OUT_Y_MSB */
bool button1 = (0U == GPIO_ReadPinInput(BOARD_SW1_GPIO, BOARD_SW1_GPIO_PIN));
bool button2 = ReadTouchButton();

g_UsbDeviceHidGamepad.buffer[0] = (uint8_t)ScaleMgToAxis(xMg);
g_UsbDeviceHidGamepad.buffer[1] = (uint8_t)ScaleMgToAxis(yMg);
g_UsbDeviceHidGamepad.buffer[2] = (uint8_t)((button1 ? 0x01U : 0U) | (button2 ? 0x02U : 0U));
```

`ScaleMgToAxis()` is the actual conversion this section is about: the accelerometer (in the same ±4g range and 0.488 mg/LSB scaling P1 established) can report several thousand milli-g at a hard tilt, but the descriptor's axis range is only -127 to 127. `±1000 mg` (roughly a 45-degree tilt) is mapped to full deflection, and anything beyond that clamps rather than wrapping around to a nonsense value:

```c
static int8_t ScaleMgToAxis(int16_t mg)
{
    int32_t scaled = ((int32_t)mg * 127) / 1000;
    if (scaled > 127) { scaled = 127; }
    if (scaled < -127) { scaled = -127; }
    return (int8_t)scaled;
}
```

Button 1 is a direct, active-low read of SW1 (the same pin and polarity every prior project has used). Button 2 reuses P1's touch-slider pattern exactly: sample one TSI electrode, subtract its calibrated baseline, and treat anything past a small noise-floor threshold as "touched."

## 10. Endpoint Configuration: the Interrupt IN Endpoint

```c
#define FS_HID_GAMEPAD_INTERRUPT_IN_INTERVAL (0x0AU) /* full-speed: bInterval is direct milliseconds */
```

Unlike the CRC or the report descriptor's length, this number is a design choice, not a derived fact: 10 ms matches the session plan's stated interval, and it's a reasonable middle ground for a demo device, fast enough that tilting the board feels responsive on screen, slow enough not to needlessly spam the USB bus. The mouse example this project started from used 4 ms; changing this single macro is the entire diff for that decision, everything else about how the endpoint works is unaffected.

## 11. SubClass and Protocol: a Correction, Not an Oversight

The mouse example's HID interface descriptor uses `SubClass 0x01` (Boot Interface) and `Protocol 0x02` (Mouse). These exist because the USB HID specification defines one fixed, simplified report format each for keyboards and mice specifically, so a PC's BIOS or bootloader, which has no room for a full HID report descriptor parser, can still use a boot-protocol mouse or keyboard before a real OS driver loads. A gamepad has no such standardized boot-protocol layout; `SubClass 0x00` (No Subclass) and `Protocol 0x00` (None) are the correct, standard values for any HID device that isn't specifically a boot-protocol mouse or keyboard, and `demo_code/01_gamepad_reference/usb_device_descriptor.h` sets them that way deliberately, not by leaving the mouse example's values unchanged.

## 12. Validating Enumeration

Plug the board into a USB port on any PC (via the second USB connector, not the SDA programming port used for flashing, P0 Section 4.1 covers the difference). Open **Device Manager** (Windows) or **System Information** (macOS) and confirm it appears as a HID-compliant game controller with no driver error or "unknown device" warning. Then open a browser and either navigate to a Gamepad API test page or run:

```javascript
window.addEventListener("gamepadconnected", (e) => console.log(e.gamepad));
```

Tilting the board should move the reported axes; pressing SW1 or touching the slider should toggle the corresponding buttons. If the device enumerates but the browser never fires `gamepadconnected`, the descriptor is likely malformed in a way the OS's low-level USB stack tolerates but the browser's higher-level Gamepad API doesn't, which is exactly the kind of quiet failure Section 0 warned about; Section 17 has specific things to check.

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
- `P1_Project_Manual.md` (this repository): the accelerometer, SW1, and touch slider setup this project reuses without re-deriving.

## 20. Developer Notes

- **`demo_code/01_gamepad_reference/` was compiled and verified at the byte level, not just written.** It builds cleanly (36 KB of 128 KB flash) with no warnings introduced by this project's own changes. The report descriptor's actual compiled bytes were extracted from the built ELF and checked against the intended 50-byte design; they matched exactly. It has not been flashed to physical hardware or plugged into a real PC. Unlike every prior project's "bench-test before the session" note, this one carries extra weight: USB enumeration genuinely cannot be verified any way other than plugging a real device into a real host, so a leader's real-hardware check before WS8 is not optional the way it might feel for some earlier projects.
- **PID choice:** `0x0092` was picked arbitrarily, one more than the stock mouse example's `0x0091`, purely so the two don't collide if both happen to be plugged in at once. The VID (`0x1FC9`) is NXP's real, USB-IF-assigned Vendor ID, reused from NXP's own example; this is standard, low-risk practice for a non-commercial educational device, not something a real shipped product could do (a real product needs its own purchased VID).
- **One pre-existing compiler warning** (a `sizeof` divisor using `usb_device_interfaces_struct_t` instead of `usb_device_interface_struct_t` around line 78 of `usb_device_descriptor.c`) was inherited unchanged from NXP's own original mouse example. It's harmless for a single-interface device like this one; it wasn't introduced by this project's changes and wasn't "fixed" either, to keep the diff against the verified original as small and auditable as possible.
- If a future project wants a richer report (more axes, a D-pad, more buttons), the extension point is entirely Section 8's descriptor plus the corresponding fields in `USB_DeviceHidGamepadAction()`; nothing about the generic USB stack files needs to change.

---
*IEEE Texas State University Student Branch. Connect. Build. Inspire.*
