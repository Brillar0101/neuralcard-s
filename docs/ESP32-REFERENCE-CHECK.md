# NeuralCard against the ESP32-S3-WROOM-1 reference schematic

Espressif publishes a reference schematic for the WROOM-1 module. This file walks
it line by line against what NeuralCard actually has, says where the two differ,
and gives a verdict on whether each difference matters.

Everything below was read out of a netlist exported from the current schematic,
not from memory.

## What the reference has, and what we have

| Reference | NeuralCard | Present |
|---|---|---|
| SW1 reset button on EN, R7 10k pull-up, C8 100nF | SW2 on EN, R9 10k pull-up, C4 100nF | yes |
| JP4 "Boot Option", a jumper shorting IO0 to GND | SW1 tact on IO0, R10 10k pull-up | yes |
| JP3 "USB OTG" header carrying USB_D+ and USB_D- | J2 USB-C receptacle, U5 USBLC6 ESD array | yes |
| JP1 UART header, TXD0 and RXD0 | J1 six pad programming header | yes |
| C1 22uF bulk and C3 100nF on the 3V3 rail | C9 and C10 22uF, six 100nF including one 2.8mm from the module | yes |
| Pin 40 GND and pin 41 EPAD tied to ground | both on GND | yes |
| JP2 JTAG header on IO39 to IO42 | not fitted, all four pins free | no |
| X1 32.768kHz crystal, C4 and C7 12pF, R3 and R5 | not fitted | no |
| R4 and R6, 0 ohm in series with USB D+ and D- | wired straight through | no |

The exact connections, for the two that looked missing:

    IO0     U1.27, SW1.1, SW1.3, R10.2, J1.6      SW1 to GND, R10 to +3V3
    EN      U1.3,  SW2.1, SW2.3, R9.2,  C4.1, J1.5
    USB_DP  J2.A6, J2.B6, U1.14, U5.1, U5.6       U1 pin 14 is IO20
    USB_DM  J2.A7, J2.B7, U1.13, U5.3, U5.4       U1 pin 13 is IO19

IO19 and IO20 are the ESP32-S3's native USB pins, so the USB-C connector is wired
to the same peripheral the reference calls USB OTG. The reference breaks it out to
a header because a module evaluation board has no connector of its own. This card
has the connector, which is the same thing one step further along.

## Boot and reset, and the open copper ring idea

The reference draws boot as JP4, a two pin jumper you bridge to hold IO0 low
through reset. NeuralCard uses a momentary tact switch instead. Electrically they
are the same node with the same 10k pull-up. A button is the better choice on a
card someone is holding, because entering download mode is hold BOOT, tap RESET,
release, and that is a one handed action with buttons and a fiddly one with a
jumper and tweezers.

Solder pads, the open copper ring idea, would save two parts and about 1.5mm of
height. They are the right answer on a board that is programmed once at assembly
and never again. They are the wrong answer here, because this card is meant to be
reflashed by whoever is holding it.

Worth knowing either way: on the S3 you rarely touch either control. The built in
USB Serial JTAG peripheral can put the chip into download mode over the USB-C
cable with no buttons pressed at all. SW1 and SW2 are the fallback for when
firmware has crashed the USB stack badly enough that the automatic path fails.
That is also why replacing them with pads would not be fatal, only annoying.

## Is anything genuinely missing worth adding

Three items in the reference are not on this board. None of them should be added.

**JTAG header, IO39 to IO42.** Not necessary. The S3 has a USB Serial JTAG bridge
built into the same D+ and D- lines already routed to the USB-C connector, so you
get JTAG debugging through the cable that is already plugged in. An external JTAG
header only earns its space when you need faster or more reliable debug than the
internal bridge gives, which is a mass production or silicon bring up problem, not
a business card problem. All four pins are still free if that ever changes.

**32.768kHz crystal.** Not necessary, and the reference agrees: every part in that
sub circuit is marked NC, no component. It only buys an accurate real time clock
across deep sleep. This card is awake when powered and does not keep time.

**0 ohm series resistors on USB D+ and D-.** Not necessary. They exist so a
prototype can be cut open for probing or impedance tuning. At USB full speed over
a trace this short, they are ceremony. Leaving them out removes two parts and two
solder joints from the highest speed pair on the board.

## A stale netlist, and why it did not matter as much as it looked

`hardware/NeuralCard.net` was stale. It predated the USB-C work and showed U1 pins
13 and 14 as unconnected, with no USB_DP, USB_DM, VBUS, CC1 or CC2 nets anywhere.
Reading it was what first suggested the USB port might be unwired.

It is regenerated now, and carries all 31 real nets. The refresh added CC1, CC2,
USB_DM, USB_DP, VBAT_SW and VBUS, and lost nothing.

The scare was smaller than it first appeared. `*.net` is in `.gitignore` and the
file has never been tracked, which is correct: a netlist is derived from the
schematic and regenerating it is one command. So nobody cloning this repository
ever saw the stale version, and the board was never wrong either. The only thing
at risk was a local tool, or a person, reading a file that KiCad had not rewritten
since the USB-C section was added.

The lesson worth keeping is to export the netlist fresh before trusting it, rather
than reading whatever happens to be sitting in the working directory.

Checked after regenerating, comparing every pin in the schematic against every pad
on the board: 211 board pads carry nets, with zero net name disagreements and zero
schematic pins without a matching pad. ERC 0 violations, DRC 0 violations and 0
unconnected.
