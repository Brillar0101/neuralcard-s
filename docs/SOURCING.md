# Component sourcing for the PCBWay assembly run

This fork is prepared for assembly by PCBWay under their maker sponsorship
program. The upstream BOM was written against JLCPCB's in-house parts library, so
every line carried an LCSC part code and nothing else. PCBWay does not buy from
that library, so this file records where PCBWay actually gets parts and confirms
that every component on the board is reachable through one of those channels.

The order-ready file is [`fab/BOM_PCBWay.csv`](../fab/BOM_PCBWay.csv). The original
`fab/BOM_JLCPCB.csv` is kept unchanged so the two can be diffed.

## Where PCBWay buys components

PCBWay is not a parts trader. They procure per project, only against an assembly
order, through two channels.

The first is authorized Western distributors: Digi-Key, Mouser, Farnell element14,
Arrow, and Avnet. Anything with a real manufacturer part number and global stock
comes from here. Budget 5 to 7 business days for procurement, stretching to 7 to
10 if customs is slow.

The second is local Shenzhen distributors. This is their stated first preference,
because local stock is immediate and skips the import leg. This channel covers the
China market parts (HRO connectors, XKB switches, Uniroyal passives, KENTO LEDs)
that never appear in a Digi-Key search. LCSC part codes are still a usable pointer
here even though PCBWay is not LCSC, because these are the same commodity parts
moving through the same Huaqiangbei supply chain.

Four arrangements are on offer: PCBWay buys everything, PCBWay buys from suppliers
you name, you consign all parts, or a mix. Turnkey is the right choice for this
board. The BOM is small and entirely commodity, and consigning parts would add
shipping and customs on our side for no saving.

Their substitution policy is the one that shapes the BOM: no substitutes without
your approval. Anything ambiguous becomes an email round trip and a delay, so the
BOM pre-approves alternates wherever a swap is safe and forbids it where it is not.

## Where the sourcing data lives

Every placed part in `NeuralCard.kicad_sch` and `NeuralCard.kicad_pcb` now carries
three hidden fields next to the existing MPN, Manufacturer, and LCSC fields:

| Field | Holds |
|---|---|
| `Alt MPN` | approved drop-in replacement, or empty where none exists |
| `Alt Mfr` | manufacturer of that alternate |
| `Sourcing` | which channel to buy from, plus the substitution rule |

The decision travels with the design instead of living only in a spreadsheet, so a
future export cannot lose it. Both files pass DRC (0 violations, 0 unconnected) and
ERC (0 violations) after the change, which is expected since the fields are hidden
text on the fab layer and touch no copper.

## Parts that need a decision before ordering

21 unique parts, 61 placements. Three need an answer from PCBWay first.

### U3, the 3.3V regulator

The fitted part is an ME6211C33M5G-N from Nanjing Micro One. It is stocked at LCSC
and throughout the Shenzhen market, but Digi-Key and Mouser do not carry it at all.

If PCBWay quotes it from local stock, take it. If they push back or want to buy
Western, the approved substitute is an AP2112K-3.3TRG1 from Diodes Incorporated. It
has the identical SOT-23-5 pinout (1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT) and is rated
600 mA against the ME6211's 500 mA. Neither choice changes the board.

### U4, the NFC tag

The schematic fits an ST25DV04KC-IE6S3, which is the active part. The older
ST25DV04K-IER6S3 is marked not recommended for new designs by ST but is still
deeply stocked (Mouser showed over 38,000 units). Both are SO-8 with the same
pinout, so either will build.

Worth knowing: `fab/BOM_JLCPCB.csv` had drifted to the older ST25DV04K while the
schematic still specified the KC, because the K was substituted in at order time
when the KC ran low at LCSC specifically. That constraint does not apply to
PCBWay's channels. `BOM_PCBWay.csv` follows the schematic and asks for the KC, with
the K listed as the fallback.

### D1 to D24, the LEDs

All 24 must come from one part number and one brightness bin. The board displays
neural network activations as relative LED brightness, so a mixed bin reel makes
the output physically wrong rather than merely uneven. This is the one line where a
well meant substitution would quietly break the project, and it is called out in
the BOM remarks and the `Sourcing` field for that reason.

## Parts sourced locally, no concerns

J2 (USB-C receptacle), SW1 and SW2 (tactile switches), SW3 (slide switch), and BT1
(coin cell holder) are standard China market parts with deep local stock.

For three of them the footprint is specific to that exact body, so a substitute
needs a land pattern review first. The land patterns were measured out of the
footprint files rather than read off a datasheet:

| Ref | Land pattern | Substitution |
|---|---|---|
| BT1 | two 3.0 by 4.2 mm pads at +/-14.5 mm, 29 mm centres | check any candidate against this |
| SW1, SW2 | four pads 1.0 by 0.75 mm at (+/-3.0, +/-1.85) mm, 5.1 mm body, 1.5 mm high | any 4 pad SMD tact of this pitch fits |
| SW3 | 4 shield tabs plus 2 NPTH locating holes of 0.85 mm | none, a different switch will not seat |

J2 is the exception with a verified alternate. An XKB U262-161N-4BVC11 is
mechanically drop-in for the fitted TYPE-C-31-M-12: NPTH pegs at +/-2.89 against
+/-2.90 mm, shield legs at +/-4.32 against +/-4.33 mm, the same 4.18 mm shield row
spacing, and identical 0.5 mm signal pad X positions. The two land patterns differ
only in how long the signal pads are drawn, which is a library convention and not a
fit constraint.

The CR2032 cell itself is not on the BOM. Holders ship empty, so buy cells separately.

## Parts with global stock, no concerns

U1 (ESP32-S3-WROOM-1-N8R2), U2 (LSM6DS3TR-C), U5 (USBLC6-2SC6), and Q1 (AO3401A)
are carried by Digi-Key, Mouser, and LCSC alike.

Two cautions on U1. Never accept an ESP32-S3-WROOM-1U, because the U suffix is the
IPEX external antenna variant and this board's antenna keepout is cut for the PCB
antenna module. The memory variant is flexible: N8R2 is the fitted part and N16R8
is an acceptable upgrade if it is better stocked or similarly priced.

U2 is a 0.5 mm pitch LGA-14 with no exposed leads. It is the one part that genuinely
needs a stencil and a real reflow profile, which is a good argument for letting
PCBWay assemble the board rather than hand soldering it. It also cannot be
substituted at all, because the firmware binds its WHO_AM_I value and register map.

Q1 has an approved alternate in the DMG2301L from Diodes Incorporated. Any -30V P
channel MOSFET in SOT-23 with a gate threshold below 1.5 V will do, since it only
has to pass gate at 3.3 V logic.

## Passives

The 0603 and 0805 resistors and capacitors are commodity, and PCBWay house stock is
fine for all of them. The BOM lists a representative MPN plus an "any equivalent"
alternate so procurement never blocks on an exact brand.

Two exceptions. C12 must be C0G/NP0 dielectric and not X7R, because it tunes the
NFC antenna and an X7R part's voltage and temperature coefficients would detune the
resonance. The target is 62 pF with a workable 56 to 68 pF window, and the value may
be retrimmed after measuring an assembled board with a VNA.

R7 and R8 are the USB-C CC1 and CC2 pulldowns. 5.1k is fixed by the USB-C
specification and is what identifies this board as a sink. A close enough 5.6k can
stop some chargers enumerating it.

## What to send PCBWay

1. Gerbers and drill from `fab/`. The existing zip has a JLCPCB name but holds
   standard Gerber X2 and Excellon files, which PCBWay reads unmodified.
2. BOM: `fab/BOM_PCBWay.csv`.
3. Pick and place: `fab/NeuralCard-cpl.csv`, 61 placements, matching the BOM exactly.
4. Board specs: 85.6 by 54 mm, 2 layer, 1.6 mm FR-4, HASL or ENIG.
5. Assembly on both sides. LEDs on top, everything else on the bottom. One through
   hole part (BT1) plus the mixed SMD and through hole legs on J2 and SW3.
6. Stencil: yes, needed for the LGA-14 at U2 whoever assembles it.

Ask them to confirm the U3 and U4 lines specifically before the order is built,
since those are the two where their answer changes which part gets placed.
