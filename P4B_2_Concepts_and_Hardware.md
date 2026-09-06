# P4-B: Concepts and Hardware

*Part of the P4-B manual split. See `P4B_1_Start_Here.md` for the full file list and how to use this manual.*

---

## 1. New Concepts You Need Before Starting

Builds on P1 (the accelerometer, touch slider, and interrupts this project reuses directly) and P2/P3 (both complete, per the prerequisite, for general comfort with peripheral configuration and interrupt-driven firmware). Everything below is new to P4-B.

**USB enumeration.** The sequence a USB device and host go through the instant a device is plugged in: the host resets the device, asks for its device descriptor (a small summary: vendor, product, device class), assigns it a unique address on the bus, asks for its configuration descriptor (a longer structure describing interfaces and endpoints), and finally activates that configuration. Only after all of this succeeds does the device actually start working. Every step is a request-response exchange on endpoint 0 (the "control endpoint"), which every USB device has by definition.

**Descriptors, and why they're layered.** A device descriptor is small and generic (this device exists, here's broadly what it is). A configuration descriptor is bigger and contains one or more interface descriptors (what distinct functions this device offers). An interface can further contain class-specific descriptors, for a HID device, that means a HID descriptor plus a report descriptor. This layering is what lets a single USB device present as several distinct pieces of functionality if needed (a webcam with a microphone, for example, is two interfaces in one device); this project only ever has one interface, but the same layered structure is still there.

**The HID report descriptor.** A separate, compact binary language (not free text, not readable without a decoder) that tells the host, precisely, byte and bit, what a HID device's reports contain: how many bytes, which bits mean what, and their valid range. Windows, macOS, Linux, and every browser all include a generic HID report descriptor parser built into the OS itself, which is the entire reason HID devices need no driver: the OS doesn't need to know in advance what a "gamepad" or "custom sensor device" looks like, it reads the description the device itself provides at enumeration time and configures itself on the spot. This is also why getting the descriptor wrong is a uniquely quiet kind of bug: the OS will happily accept and try to use a malformed descriptor, just incorrectly.

**HID vs. CDC, and why one needs a driver and the other doesn't.** CDC (Communication Device Class, what a virtual serial port uses, like this board's own OpenSDA connection) has a fixed, simple report format the class spec itself defines, but interacting with it as "a serial port" requires OS-specific driver glue to expose it that way to applications. HID's self-describing report descriptor means the OS can build a generic, correct interface to the device without ever needing device-specific code: this is a deliberate design trade-off in the USB HID specification, not an accident, and it's exactly why this project's descriptor design work is the thing that makes driver-free operation possible at all.

**Endpoints.** A USB device's control endpoint (endpoint 0) handles descriptor requests and configuration; separate from that, most devices have one or more additional endpoints for their actual data. This project uses one interrupt IN endpoint: "interrupt" here doesn't mean a CPU interrupt, it's a USB transfer type meaning the host polls this endpoint at a fixed interval (Section 10) and the device replies with its latest data whenever asked.

**Sensor-to-report mapping.** The board's actual sensors (14-bit accelerometer readings in milli-g, a digital button state, a touch-slider threshold) don't naturally look like a HID gamepad report (signed 8-bit axes, single-bit buttons). Converting from one representation to the other, correctly, with sensible clamping at the edges, is genuine engineering work, covered in Section 9.

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

---

**Next:** `P4B_3_Setup_and_Walkthrough.md` for the hands-on session steps.

---
*IEEE Texas State University Student Branch. Connect. Build. Inspire.*
