# TXST IEEE Student Branch: FRDM-KL26Z Project Series
## Capstone Manual, P4-B: USB HID Gamepad

**Document status:** DRAFT v0.1, for project-leader review and bench testing before member use
**Track:** Embedded Systems | **Difficulty:** Hard | **Sessions:** 1 (WS8, Nov 5, 2026), plus independent build before the Nov 19 demo
**Groups:** 2 groups of 3, working independently
**Companion documents:** P0's manual (`P0_Board_Orientation_and_Toolchain_Setup/`, start at `P0_1_Start_Here.md`) through P3's manual (`P3_DMA_ADC_FIR_Filter/`) (read all four first), *TXST IEEE FRDM-KL26Z Project Specification*
**Hardware:** FRDM-KL26Z only (uses the on-board full-speed USB controller)
**Demo code:** see `demo_code/` in this project's folder

---

## This Manual Is Split Into 4 Files

Long single files invite procrastination. Read only what you need, when you need it:

1. **`P4B_1_Start_Here.md`** (this file) — how to use this manual, why P4-B exists, purpose, prerequisites.
2. **`P4B_2_Concepts_and_Hardware.md`** — background theory (USB enumeration, descriptors, HID, endpoints) and the hardware/software reference. Read once if any term below is new to you.
3. **`P4B_3_Setup_and_Walkthrough.md`** — the actual hands-on steps. **This is the file you follow during the session.**
4. **`P4B_4_Reference.md`** — code structure explanation, sample output, session plan, milestones, debugging table, glossary, references, developer notes. Look things up here when stuck.

Section numbers (0-20) are kept consistent across all 4 files, so "see Section 8" always means the same section no matter which file you're in.

---

## How to Use This Manual

Same rule as every prior manual, with the same extra emphasis P4-A gave the whiteboard step: this project's entire point is understanding the HID report descriptor well enough to have designed one yourself, not having a working binary. Try designing your own descriptor and watching enumeration fail in informative ways before opening `demo_code/`.

Project leaders: bench-test `demo_code/01_gamepad_reference/` end to end, on a real board plugged into a real PC, before WS8. USB enumeration is uniquely unforgiving to debug blind: when something is wrong, the usual symptom is not an error message, it's the device simply not showing up, or Windows silently falling back to a generic (and wrong) driver. A leader who has seen this exact firmware enumerate correctly once has something concrete to compare a stuck group's board against.

**A note on accuracy:** this project was built by starting from NXP's own shipped, working USB HID mouse example, not by writing USB stack code from scratch, because the generic USB enumeration and HID class-request handling in that example needs zero changes to work for any HID device, gamepad included. What actually changed (the report descriptor, the SubClass/Protocol codes, the sensor-to-report mapping) is documented field by field in this manual, and the compiled binary's actual descriptor bytes were extracted and checked against the design after building, not just assumed correct. Section 21 has the full trail.

---

## 0. Why This Session Exists

Every device you've plugged into a computer, a mouse, a keyboard, a flash drive, went through a conversation with the operating system before it did anything useful: "here's what kind of device I am, here's what I can do, here's how my data is structured." That conversation is USB enumeration, and for the specific case of HID (Human Interface Devices), it's also why a mouse or a keyboard just works the instant you plug it in, no driver install, no restart. P4-B is where you make the FRDM-KL26Z hold up its end of that conversation convincingly enough that a completely unmodified PC believes it's a real gamepad. This is a genuinely different kind of hard than the earlier capstone: there's no compiler error waiting for you if you get a descriptor field wrong, only a device that mysteriously doesn't work, or works partially, or works on one OS and not another. Patience and careful, byte-by-byte verification, the habit this whole manual series has tried to model, matter more here than almost anywhere else in the curriculum.

## 2. Purpose

By the end of this project, every group has: a HID report descriptor they understand well enough to explain field by field; a board that enumerates on any PC as a driver-free gamepad; two absolute axes driven by real accelerometer tilt; two buttons driven by the push button and the touch slider; and a live demonstration on a browser-based Gamepad API tester at the Nov 19 showcase.

## 3. Prerequisites

P1 (I2C accelerometer, touch slider, interrupts), P2, and P3 all complete. This project reuses P1's exact, already-verified accelerometer I2C sequence, SW1 button setup, and TSI touch slider code without re-deriving any of it; if those feel shaky, revisit P1's manual first.

---

**Next:** `P4B_2_Concepts_and_Hardware.md` for the concepts and hardware reference, or skip straight to `P4B_3_Setup_and_Walkthrough.md` if you're already comfortable with USB enumeration and HID descriptors.

---
*IEEE Texas State University Student Branch. Connect. Build. Inspire.*
