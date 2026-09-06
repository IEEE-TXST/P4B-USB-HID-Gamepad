# P4-B: Setup and Walkthrough

*Part of the P4-B manual split. See `P4B_1_Start_Here.md` for the full file list and how to use this manual.*

---

## 4. Additional Toolchain for This Project

Everything from prior projects still applies. New for P4-B: **any modern browser** (Chrome, Firefox, Edge) to validate enumeration via its built-in Gamepad API, no install needed; a page like `gamepad-tester.com`, or a small local HTML page calling `navigator.getGamepads()`, both work. **USBPcap and Wireshark** are optional, for groups who want to actually watch the enumeration traffic and see the report descriptor bytes as the OS parses them; not required for the exit criteria, but genuinely illuminating if a group gets stuck and wants to see exactly what the host is asking for and how the board answers.

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

---

**Next:** `P4B_4_Reference.md` for code structure, sample output, debugging, and the glossary.

---
*IEEE Texas State University Student Branch. Connect. Build. Inspire.*
