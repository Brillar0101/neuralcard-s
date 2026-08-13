# Component sourcing for the PCBWay assembly run

This fork exists to be assembled by PCBWay under their maker sponsorship program.
The upstream BOM was written against JLCPCB's in-house parts library, so every line
carried an LCSC part number and nothing else. PCBWay does not buy from that library,
so this document records where PCBWay actually gets parts and confirms that every
component on this board is reachable through one of those channels.

The order-ready file is [`fab/BOM_PCBWay.csv`](../fab/BOM_PCBWay.csv). The original
`fab/BOM_JLCPCB.csv` is kept unchanged so the two can be diffed.

## Where PCBWay buys components

PCBWay is not a parts trader — they procure per project, only against an assembly
order. They buy through two channels:

**Authorized Western distributors.** Digi-Key, Mouser, Farnell element14, Arrow, and
Avnet. Anything with a real manufacturer part number and global stock comes from here.
Expect 5–7 business days for procurement, stretching to 7–10 if customs is slow.

**Local Shenzhen distributors.** This is their stated first preference, because local
stock is immediate and avoids the import leg. This channel covers the China-market
parts — HRO connectors, XKB switches, Uniroyal passives, KENTO LEDs — that never
appear in a Digi-Key search. LCSC part numbers are a usable pointer here even though
PCBWay is not LCSC, because these are the same commodity parts moving through the same
Huaqiangbei supply chain.

You get four sourcing arrangements: PCBWay buys everything (turnkey), PCBWay buys from
suppliers you name, you consign all parts, or a combination. For this project turnkey
is the right choice — the BOM is small and entirely commodity, and consigning parts
would add shipping and customs on our side for no saving.

Their substitution policy is the important one: **no substitutes without your approval.**
That means anything ambiguous in the BOM turns into an email round-trip and a delay, so
the PCBWay BOM below pre-approves alternates wherever a substitution is actually safe,
and explicitly forbids it where it is not.

## Per-part sourcing status

21 unique parts, 61 placements. Three need a note before ordering; the rest are routine.

### Needs attention

**U3 — ME6211C33M5G-N (3.3V LDO).** This is a Nanjing Micro One part. It is stocked at
LCSC and throughout the Shenzhen market, but it is *not* carried by Digi-Key or Mouser.
If PCBWay quotes this from local stock, take it. If they push back or quote a Western
distributor, the approved substitute is **AP2112K-3.3TRG1** (Diodes Inc), which has the
identical SOT-23-5 pinout — pin 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT — and a higher 600 mA
rating. No board change is needed either way.

**U4 — ST25DV04K-IER6S3 (NFC tag).** Stocked and shipping (Mouser showed 38k+ units),
but ST has marked it *not recommended for new designs*. The active successor is
**ST25DV04KC-IE6S3**, same SO-8 package and same pinout, also stocked. Either part works
on this board. Prefer the KC if both are quoted, since it is the part with a future.
This is the reverse of the upstream decision, which picked the K because the KC was
running low specifically at LCSC — that constraint does not apply to PCBWay's channels.

**D1–D24 — KT-0603R red LEDs.** All 24 must come from a single part number and a single
brightness bin. This board displays neural network activations as relative LED
brightness, so a mixed-bin reel makes the output physically wrong, not merely uneven.
This is the one line where a well-meaning substitution would quietly break the project,
and it is called out in the BOM remarks for that reason.

### Sourced locally, no concerns

J2 (TYPE-C-31-M-12 USB-C receptacle, HRO), SW1/SW2 (TS-1187A tactile, XKB), SW3
(MSK12C02 slide switch, Shouhan), and BT1 (CR2032-BS-6-1 holder) are all standard
China-market parts with deep local stock. None of them have a drop-in Western
equivalent, because in each case the *footprint* is specific to that exact body — the
USB-C land pattern, the 3.7 mm tact pitch, the slide switch's 0.85 mm NPTH alignment
pegs, and the coin cell holder's pin spacing are all designed around these parts. These
must not be substituted without a land pattern review. Local sourcing is both the
cheapest and the safest path here.

Note that the CR2032 cell itself is not on the BOM. Holders ship empty; buy cells
separately.

### Globally stocked, no concerns

U1 (ESP32-S3-WROOM-1-N8R2, Espressif), U2 (LSM6DS3TR-C, ST), U5 (USBLC6-2SC6, ST), and
Q1 (AO3401A, Alpha & Omega) are carried by Digi-Key, Mouser, and LCSC alike.

Two cautions on U1. Do not accept an **ESP32-S3-WROOM-1U** — the U suffix is the IPEX
external-antenna variant, and this board's antenna keepout is cut for the PCB-antenna
version. The memory variant is flexible: N8R2 is the designed part, and N16R8 is an
acceptable upgrade if it happens to be better stocked or similarly priced.

U2 is a 0.5 mm pitch LGA-14 with no exposed leads. It is the one part on the board that
genuinely needs a stencil and a proper reflow profile, which is a good argument for
letting PCBWay assemble it rather than hand-soldering. It also cannot be substituted at
all: the firmware reads its WHO_AM_I value and register map directly.

### Passives

The 0603 and 0805 resistors and capacitors are commodity. PCBWay house stock is fine for
all of them, and the BOM lists a representative MPN plus an "any equivalent" alternate
so procurement is never blocked on an exact brand.

Two exceptions worth stating:

- **C12 (68 pF)** must be C0G/NP0 dielectric, not X7R. It tunes the NFC antenna, and an
  X7R part's voltage and temperature coefficient would detune the resonance. The design
  target is 62 pF with a workable 56–68 pF window, and the final value may be adjusted
  after measuring an assembled board with a VNA.
- **R7 and R8 (5.1 k)** are the USB-C CC1/CC2 pulldowns. 5.1 k is fixed by the USB-C
  specification and identifies this board as a sink. A "close enough" 5.6 k can cause
  some chargers to not enumerate it.

## What to send PCBWay

1. Gerbers and drill: `fab/` (the existing zip is a JLCPCB-named bundle of standard
   Gerber X2 + Excellon files, which PCBWay reads without modification).
2. BOM: `fab/BOM_PCBWay.csv`.
3. Pick and place: `fab/NeuralCard-cpl.csv` — 61 placements, matching the BOM exactly.
4. Board specs: 85.6 × 54 mm, 2-layer, 1.6 mm FR-4, HASL or ENIG.
5. Assembly: top and bottom (LEDs on top, everything else on bottom), 1 THT part
   (BT1 coin cell holder) plus the mixed SMD/THT legs on J2 and SW3.
6. Stencil: yes — needed for the LGA-14 at U2 regardless of who assembles.

Ask them to confirm the U3 and U4 line items specifically before the order is built,
since those are the two where their answer changes which part gets placed.
