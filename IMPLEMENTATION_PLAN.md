# Math Game Curriculum Redesign

## Why this redesign

The current game (`src/game.c`) is addition-only: two random digits 0-9,
keyboard entry, right/wrong feedback. Published math-education research says
several prerequisite and intermediate steps are missing before addition, and
several conceptual sub-models are missing within each operation. This plan
reorders the curriculum around that research and describes the engineering
changes needed to support it on the existing bare-metal Pi codebase
(raw fbdev/evdev, `viewport.h` virtual-coordinate rect/line drawing, FreeType
text via `ttf_font.h`).

**Target age is 4+.** Because progression is mastery-gated rather than
fixed-schedule, the same curriculum spans a wide age range by construction —
a child only advances once they've demonstrated the current stage, so a
4-year-old and a 7-year-old both just enter at Stage 1 and move at their own
pace. What actually changes for a lower age floor is the *entry point*
inside Stage 1 (a 4-year-old's subitizing/counting range starts smaller than
a 5-year-old's) and some interaction-pacing details — see Stage 1 below.
Later stages (multiplication, division) remain here for when a child's
progress reaches them; "keep improving for later age groups" means adding
breadth/depth at the top over time, not blocking the bottom on it.

### Research basis (see citations at the end)

- **Number sense before arithmetic.** Kindergarten number sense (counting,
  subitizing, nonverbal calculation) predicts up to ~66% of first-grade math
  achievement variance (Jordan et al., 2006-2007). Subitizing and counting
  develop together and should be built explicitly (Clements & Sarama).
  Counting isn't just number-word recitation — a counting stage must test
  *cardinality* ("how many in total?"), not just sequence (Gelman & Gallistel).
- **Concrete-Pictorial-Abstract (CPA).** Singapore Math / Bruner: introduce
  each new concept with a visual/iconic representation before pure symbolic
  digit entry, and fade the visual as mastery builds. The current game skips
  straight to symbolic entry for every concept.
- **Addition strategy progression.** Counting-on → doubles/near-doubles →
  make-ten/bridging-to-ten (Carpenter et al.; Common Core OA progression).
  Make-ten is the pivotal bridge to multi-digit addition.
- **Subtraction has two models**, take-away and difference/comparison, and
  schools under-teach the comparison model, which weakens the
  addition-subtraction inverse relationship later (Selter et al., 2012;
  Carpenter & Moser, 1984). Both should be taught, plus missing-addend framing.
- **Multiplication progression**: equal groups/repeated addition → skip
  counting → arrays (arrays visually unify the first two and set up later
  area-model multiplication).
- **Division has two models**, partitive/sharing and quotitive/grouping, and
  they are not interchangeable for young children — sharing is understood
  earlier (Correa, Nunes & Bryant).
- **Grade-level anchors**: UK EYFS ELG targets subitizing-to-5 and number
  bonds to 10 by end of Reception (~age 5); US Common Core puts addition/
  subtraction within 20 with strategies in Grade 1, fluency within 20 plus
  double-digit regrouping in Grade 2, and multiplication/division (single-digit
  fluency) in Grade 3.
- **Age-4 floor below those anchors.** UK "Birth to 5 Matters" (the
  non-statutory guidance covering ages 3-4, one year *before* the Reception
  ELGs above) targets fast recognition of up to 3 objects without counting,
  reciting numbers past 5, one-to-one counting to 5, and matching numerals to
  amounts up to 5 — a full number lower than the age-5 ELG. Clements &
  Sarama's subitizing trajectory has most children reliably naming groups of
  1-3 around 34-39 months and reaching "Perceptual Subitizer to 4" about six
  months later — i.e. instant recognition of 4 is a distinct, later
  milestone than 1-3, not the same skill scaled up.
- **No countdown timers / visible time pressure.** Boaler's research links
  timed tests to early-onset math anxiety via a stress-consumes-
  working-memory mechanism (Beilock), and this risk is if anything higher
  for a 4-year-old than a 7-year-old. This game must never show a ticking
  countdown or fail a child for running out of time.
- **Feedback must be corrective, not punitive.** Young children don't
  discriminate feedback intensity well — generic "wrong" or loud negative
  stingers land harder than intended. Feedback should show *what the right
  answer is*, not just pass/fail (Computers & Education: AI, 2023).
- **Mastery-based, not fixed-count, progression**, with immediate (not
  batched) feedback (Bloom mastery learning; RCTs on adaptive math games,
  2021-2022).
- **Spaced + interleaved practice**, not pure random generation or blocked
  practice: recently-missed facts should resurface sooner than mastered ones
  (Bjork's "desirable difficulties"), and once multiple problem types coexist
  they should be mixed rather than drilled in long same-type blocks — mixing
  forces the child to identify *which* operation/strategy applies, not just
  execute a memorized procedure (Rohrer & Taylor; Rohrer, Dedrick & Stershic).

### Decisions already made (per your answers)

- Render proper `×` and `÷` glyphs, not ASCII stand-ins — requires extending
  `ttf_font.c`, which currently loads one `unsigned char` per glyph
  (`FT_Load_Char(g_face, *p, ...)` in `ttf_font.c:48,67`), so any UTF-8
  multi-byte sequence (e.g. `×` = U+00D7 = bytes `0xC3 0x97`) is decoded as
  two wrong glyphs today.
- Counting/subitizing stages render actual visual quantities (dot groups),
  not just numerals.
- Stage progression is mastery-based (accuracy threshold over a rolling
  window), not a fixed problem count.
- The physical device's answer input is two NFC pads holding numeral tags
  (0-9, multiple physical copies of each digit) read as a tens/ones
  place-value pair, confirmed by a dedicated physical button — not typed
  keyboard/keypad entry. See "Physical Hardware Platform" below.

### Non-goals for this plan (explicitly deferred)

- Division with remainders, negative numbers, fractions — out of scope for
  this age range and not requested.
- Natural-language word problems (the platform has no text input beyond
  digits/enter/backspace; would need a much larger input/rendering rework).
- Audio feedback — the current platform layer has no audio output; adding it
  is a separate, larger change and isn't required by the research to be
  effective (visual corrective feedback suffices).

---

## Physical Hardware Platform

The game is being packaged as a standalone arcade-style device: a salvaged
laptop LCD panel (HDMI input, so still just a framebuffer target — no change
to the `viewport.h` scaling approach, which already queries real screen size
and letterboxes to fit), arcade pushbuttons, two PN532 NFC readers with a set
of 0-9 numeral tags (multiple physical copies per digit), a WS2812-style
individually-addressable RGB LED strip, and a wood/MDF enclosure. Full pin
assignments and an ASCII wiring diagram are in `WIRING.md` at the repo root.

### Research basis for going physical

- **Tangible interaction outperforms screen-only for young children's math.**
  A systematic review of 155 studies on TUIs in early-years education (Dujić
  & Granić, *Personal and Ubiquitous Computing*, 2021/2022) and a head-to-head
  study of a tangible vs. touchscreen version of the same counting app found
  the tangible version produced more task attempts (itself a predictor of
  learning outcome), lower cognitive load, and higher engagement. This
  directly supports building this as a tangible device rather than a
  screen+keyboard one.
- **Design framework**: Antle & Wise's tangible-design themes for children
  ages 4-12 — "space for action" (the physical interaction must give the
  child real control) and "perceptual mappings" (the link between a physical
  action and its digital effect must be obvious) — is the reason the NFC pads
  are fixed, clearly-labeled tens/ones slots rather than a single ambiguous
  pad, and why LED/screen feedback should fire immediately on each tag
  placement, not just at Confirm.
- **This is a validated interaction pattern, not a novel one.** Academic
  work on NFC-tagged-object learning systems for children (Sánchez, Cortés,
  Riekki & Oja, ACM IDC 2011) explicitly validates numeracy as a use case for
  exactly this pattern (NFC reader + tagged physical objects); commercial
  precedent (Ravensburger tiptoi, LeapFrog Tag) uses the same
  "touch a physical object → get instant feedback" principle via optical
  codes instead of NFC. The interaction-design lesson from that prior art:
  keep tag-read latency low and the placement zone physically unambiguous
  (both products use a fixed, constrained placement area, matching the
  pad-based design here).
- **Recognition beats recall at this age — this makes tile-selection a
  pedagogical upgrade, not just a hardware substitute.** Developmental memory
  research consistently finds young children perform far better recognizing
  a correct option among alternatives than freely recalling/producing one;
  free verbal/typed recall in 4-6-year-olds is frequently "at floor" while
  spatial/pointing-style recognition is a far more sensitive and successful
  measure of what a child actually knows. Selecting and placing the correct
  numeral tile is a recognition task; typing a remembered digit sequence on a
  keyboard is closer to free recall. This means the tile-based answer input
  isn't just more tangible, it's better matched to what this age group can
  actually reliably do.

### Answer input: physical place-value tiles, not typing

Confirmed design: the two NFC pads are fixed **tens** and **ones** place-value
slots. To answer, a child places the numeral tag for each digit of their
answer on the matching pad (single-digit answers just use the ones pad) and
presses a dedicated **Confirm** arcade button to submit — replacing keyboard
digit entry on the physical device entirely. This is a genuine pedagogical
upgrade, not just a keyboard substitute: composing "23" by physically placing
a "2" tile on the tens pad and a "3" tile on the ones pad *is* the concrete
stage of Concrete-Pictorial-Abstract for place value (already cited above:
Leong, Ho & Cheng; Bruner) — more so than the on-screen base-ten-block visual
alone, which can now be presented as reinforcement rather than the only
concrete representation. Because a tag can be freely swapped before pressing
Confirm, no separate "clear" control is needed for v1.

This also resolves Stage 0's answer-buffer question in the simpler direction:
since only two place-value pads exist, the physical device can only ever
compose answers 0-99, so **operand ranges across every stage should be chosen
so results never exceed 2 digits** — this isn't a limitation forced by
hardware so much as a match to how the standards already scope this age
range (Common Core 2.NBT frames double-digit addition/subtraction as
explicitly "within 100"). See the revised Stage 3 scope below.

The keyboard/SDL dev-loop input path is retained for fast iteration when
hardware isn't attached — game/curriculum logic must only ever see a
finished "answer value + confirm" event, never care whether it came from
keystrokes or NFC tiles, so no stage's logic needs hardware-specific code.

### LED feedback: celebratory and ambient, never alarming

Per the research already established for on-screen feedback (non-punitive,
corrective), plus color-psychology research specific to this choice — red is
consistently linked to failure/shame associations (including the well-known
"red pen" effect on schoolwork), while green/blue-green hues are associated
with calm and focus — the addressable LED strip should be used for:
- **Correct**: a brief green/warm celebratory sweep or sparkle.
- **Incorrect**: a short, gentle, non-alarming pulse (soft amber/blue — not
  red, not a harsh flash) paired with the on-screen corrective "here's the
  right answer" feedback, never framed as a loss.
- **Stage mastery**: a distinct, rarer "level-up" pattern (e.g. a full
  rainbow chase) — reserving the most exciting signal for genuine milestones
  keeps moment-to-moment wrong answers from feeling like a bigger deal than
  they are.
- **Idle/attract mode**: a slow ambient breathing animation when no one's
  playing, inviting a child to approach the device like a lit-up arcade
  cabinet.
- Optionally, per-pad indicator light (if the strip is routed near/around the
  NFC pads) showing which pad is expected for the current problem, reducing
  reliance on text for pre-readers.

### New platform-layer work

This is new engineering distinct from the curriculum-content stages, and can
proceed in parallel with them (curriculum logic is source-agnostic; it can be
built and tested entirely on the SDL keyboard backend before hardware
stages land). Tracked as its own stage sequence:

---

## Stage H1: Arcade Confirm Button

**Goal**: A physical button press generates the same "commit answer" event
the keyboard's Enter key does today.

**Scope**: If the buttons are wired through a generic USB/GPIO-to-HID encoder
board (recommended — common, cheap, and means the existing evdev
`plat_poll_key()` path in `platform_fbdev.c` already delivers button presses
as key events with zero new driver code), map the Confirm button's keycode to
a new `GK_CONFIRM` (or reuse `GK_ENTER`). If instead wired directly to GPIO,
this stage additionally needs a small polled-GPIO reader with debounce (no
existing code path for this).

**Success Criteria**: Pressing the physical Confirm button advances the game
identically to pressing Enter today, verified on real hardware.

**Tests**: manual hardware test (press button, observe answer-commit
behavior); if direct GPIO, a debounce unit test (rapid mechanical bounce
within Xms collapses to one logical press).

**Status**: Not Started

---

## Stage H2: Dual NFC Place-Value Input

**Goal**: Reading a numeral tag placed on either PN532 pad produces a digit
value the game can compose into a tens/ones answer.

**Scope**:
- Use `libnfc` against the two PN532 modules (userspace, works headless with
  no X server, fits this codebase's "talk directly to the hardware" style
  better than hand-rolling PN532's frame protocol) rather than a from-scratch
  driver.
- Wire the two PN532 modules on the Pi's SPI0 bus using its two native chip-
  selects (CE0/CE1) — avoids the I2C dual-fixed-address conflict two PN532
  boards would otherwise hit on a shared I2C bus.
- A small tag-UID → digit (0-9) lookup table, populated once when tags are
  programmed/labeled.
- Poll both readers each frame; compose `(tens_digit, ones_digit)` into an
  answer value, treating an empty pad as "no tens digit" for single-digit
  stages.
- New `plat_poll_nfc(pad_id)` -style platform function alongside the existing
  `plat_poll_key()`.

**Success Criteria**: Placing tags "2" and "3" on the tens/ones pads and
pressing Confirm submits 23; a single tag on the ones pad alone submits a
single-digit value; removing/swapping a tag before Confirm changes nothing
until Confirm is pressed.

**Tests**: UID→digit lookup unit test; place-value composition unit test
(tens-only, ones-only, both, neither); manual hardware test for actual tag
reads through the enclosure material at the intended pad recess depth (PN532
range is short — verify read reliability through the chosen MDF thickness
before finalizing the enclosure).

**Status**: Not Started

---

## Stage H3: LED Feedback Driver

**Goal**: Drive the WS2812-style strip with the pattern set described above,
hooked into Stage 0's existing feedback events (correct / incorrect / stage
mastery) so LED patterns never need to be triggered from more than one place.

**Scope**: A small pattern-playback driver (try `rpi_ws281x`, which targets
the BCM2835 peripherals this Pi has, before resorting to a bit-banged
SPI-timing workaround if the library proves unreliable on this specific
board/kernel); a fixed small set of named patterns (correct, incorrect,
mastery, idle-breathing) triggered from the same feedback path Stage 0
already routes on-screen feedback through, so LEDs and screen feedback can
never drift out of sync (e.g. a red-ish flash could never fire alongside a
"gentle" on-screen corrective message).

**Success Criteria**: Each feedback event reliably fires exactly one LED
pattern; idle-breathing runs when no problem is active; no pattern is ever
triggered by more than one code path.

**Tests**: manual hardware test per pattern; a unit test that every feedback
event type maps to exactly one pattern (no missing/duplicate mapping).

**Status**: Not Started

---

## Stage H4: Enclosure Integration & End-to-End Device Test

**Goal**: Verify the full physical loop works together once the enclosure is
built: screen, buttons, both NFC pads, and LEDs as one device.

**Scope**: Physical build (wood/MDF) is the user's own work, not something to
plan in software-engineering detail here; this stage is the integration
checkpoint once it exists. Ergonomic notes worth building in, drawn from
Nielsen Norman Group's children's-UX research and assistive-switch design
data (no formal "kids' arcade button" standard exists, so these are the best
available proxies):
- Confirm button: a large active face (NN/g's finding for ages 3-5 is
  roughly 2cm × 2cm minimum, 4x the adult-recommended target size, because
  fine motor control is still developing at this age) with generous spacing
  from any other controls, and low actuation force — standard arcade-button
  springs are tuned for adult competitive play; prefer a lighter-throw
  switch (assistive-switch products aimed at young/low-precision users
  commonly use ~5oz activation force as a reference point).
- NFC pad recess shallow enough for reliable PN532 reads (verified in
  Stage H2) with no metal hardware near the antenna area.
- Numeral tags/tiles sized generously (beyond small-parts/choking-hazard
  dimensions) given the age floor of 4 — worth checking against EN 71 / ASTM
  F963 toy-safety small-parts requirements even though this is a one-off
  build, not a commercial product.
- A rear access panel to the Pi/SD card for maintenance.

**Success Criteria**: A full problem can be answered start-to-finish using
only the physical device (no keyboard), across at least one stage from each
curriculum operation, with LED and on-screen feedback both firing correctly.

**Tests**: end-to-end manual test session on the assembled device.

**Status**: Not Started

---

## Stage 0: Curriculum Engine & Cross-Cutting Infrastructure

**Goal**: Replace the single hardcoded addition generator with a generalized
problem/curriculum framework that every later stage plugs into, so stages 1-6
are purely content, not re-plumbing.

**Scope**:
- `Problem` struct: operation type, operands, correct answer, optional visual
  representation mode (none / dots / ten-frame / base-ten blocks / array /
  grouping).
- Per-substage item generator + a **mastery tracker**: rolling window (e.g.
  last 8 attempts) per substage; advance on accuracy ≥ threshold (e.g. 6/8)
  with no fixed problem count.
- **Adaptive item scheduler**: Leitner-style buckets per fact — wrong answers
  drop to bucket 0 (resurface soon), correct answers promote (resurface
  later) — replacing `rand() % 10` with weighted-by-bucket selection.
  Once a substage has multiple strategy types active, interleave across them
  rather than blocking by type.
- **Persistence**: save current stage + mastery/bucket state to a file (the
  Pi runs this as a systemd service — `math-game.service` — so state must
  survive process restarts). Simple flat/JSON file under a fixed path.
- **Corrective feedback**: on a wrong answer, show the solved equation
  (e.g. "6 + 3 = 9") in addition to the current non-punitive
  checkmark/X-line feedback, instead of just marking it wrong.
- **Font layer**: extend `ttf_font.c` to decode UTF-8 (at minimum the 2-byte
  range covering Latin-1 Supplement, U+0080-U+00FF) so `×` (U+00D7) and `÷`
  (U+00F7) render as single correct glyphs.
- **Answer buffer**: stays at 2 digits (`game.c:69`) — the physical device's
  two NFC place-value pads can only ever compose 0-99 (see "Physical
  Hardware Platform" below), so every stage's operand ranges are chosen so
  results never exceed 2 digits. No widening needed.
- Confirm no timer/countdown mechanic is ever added to the child-facing
  answer flow (stimulus-display timing in Stage 1's perceptual-subitizing
  flash is internal pacing, not a visible countdown, and never fails/blocks
  the child's answer).

**Success Criteria**: A single generic game loop drives all six content
stages via the `Problem`/scheduler abstraction; addition (today's only
content) is reimplemented on top of it with no behavior regression; state
persists across a process restart; `×`/`÷` render correctly in a smoke test
string.

**Tests**: unit tests (host-side, compiled outside the fbdev backend) for the
mastery tracker (accuracy math, advance/no-advance boundary), the bucket
scheduler (wrong items resurface before mastered ones), and persistence
(save/load round-trip). Manual test on SDL backend: type an intentionally
wrong answer, confirm the solved equation is shown.

**Status**: In Progress. Built: `curriculum.c`/`.h` (mastery tracker +
Leitner-style scheduler), `progress.c`/`.h` (save/load), corrective feedback
("It was N" on a wrong answer) in `game.c`. Unit tests in
`tests/test_curriculum.c`, run via `make test`. Deliberately deferred: the
generic `Problem`-struct dispatch table across content types (only one
content stage exists so far — subitizing — so building a plugin abstraction
now would be speculative; do it when Stage 2 is added and there's a second
case to design it against) and the `ttf_font.c` UTF-8 decode for `×`/`÷`
(not needed until a stage that uses those glyphs).

---

## Stage 1: Number Sense — Subitizing, Counting & Cardinality

**Goal**: Build the perceptual/conceptual foundation research shows predicts
later math achievement, before any symbolic arithmetic.

**Scope**:
- Render dot groups with `vfill_rect` (small squares are fine at this scale;
  no circle primitive needed).
- Sub-phase (0) **Perceptual subitizing to 3, then 4** — the age-4 entry
  point: instant recognition of 1-3 items first (the range most 4-year-olds
  already have), extending to 4 as a distinct, separately-mastered milestone
  per Clements & Sarama's trajectory, before ever reaching 5. A child who
  enters already fluent to 4 clears this sub-phase almost immediately via the
  mastery gate rather than being held back by it.
- Sub-phase (a) **Perceptual subitizing to 5**: canonical dice-like patterns
  for n=1-5, brief internal display pacing (not a child-facing countdown —
  answering is untimed), ask "how many?". Matches the Reception/age-5 ELG
  ceiling, one step above sub-phase 0's age-4 target of 3-4.
- Sub-phase (b) **Conceptual subitizing**: composed patterns 6-10 (e.g. a
  5-group + a 2-group) to build "5 and 2 more is 7" pre-addition intuition.
- Sub-phase (c) **Counting & cardinality**: dots stay visible, untimed;
  question is explicitly "how many in total?" (not sequence recall), testing
  Gelman & Gallistel's cardinality principle directly. Starts with counting
  ranges 1-5 (Birth to 5 Matters' age 3-4 target) before extending to 10.
- Answer input: reuse existing digit + Enter flow (0-10 fits existing 2-digit
  buffer).
- Interaction pacing for younger entrants: no fixed minimum answer time
  requirement, larger on-screen text/spacing than a text-drill needs, and a
  more generous internal display duration for sub-phase 0's flash than
  sub-phase (a)'s, since younger children need more perceptual processing
  time even for smaller sets.

**Success Criteria**: Child reaches mastery threshold independently in each
sub-phase in order (interleaved once (b) and (c) are both introduced) before
Stage 2 unlocks. A child already fluent at the youngest range moves through
sub-phase 0 quickly rather than being slowed by it.

**Tests**: generator unit tests (dot count matches `n` for every pattern,
patterns don't repeat identically on consecutive prompts to avoid rote
memorization instead of counting); mastery-gate integration test that Stage 2
doesn't unlock below threshold; range-boundary test that sub-phase 0 never
generates n>4 and sub-phase (a) never generates n>5.

**Status**: Complete for the scope defined above. All four sub-phases (0,
a, b, c) are built as five mastery-gated steps in `stage_subitizing.c`
(pure step/pool logic, host-testable) and `stage_subitizing_draw.c`
(rendering, depends on `viewport.h`), wired into `game.c`'s main loop.

Implementation detail worth recording: facts accumulate in a single pool
across steps rather than being replaced step-to-step — advancing a step
only *adds* that step's new facts to the pool (via `sched_grow`, which
preserves existing buckets) rather than resetting it. Mastery for advancing
the *current* step is judged only on that step's own new facts
(`subitize_step_start_index`), while every earlier fact stays in the same
scheduler and keeps resurfacing at spaced intervals. This is what actually
delivers the "interleaved once (b) and (c) are both introduced" requirement
from the Success Criteria above, and — as a side effect of using the same
mechanism uniformly — also means sub-phase 0's facts keep getting reviewed
during (a)/(b)/(c) rather than being dropped once passed, which is a better
match for the spaced-retrieval research than the original per-phase-reset
design this replaced.

Sub-phase (b) draws a fixed 5-cluster plus a 1-5-cluster side by side (5 and
2 more is 7, etc.); sub-phase (c) draws scattered (non-canonical) dots
instead of dice-face patterns specifically so the child must actually count
them rather than pattern-match, per the perceptual/counting distinction in
the research. Input is keyboard digit + Enter (the physical NFC/button
hardware from Stage H1-H2 isn't built yet — see `WIRING.md`).

Not yet built: dedicated rendering tests for the composed/scattered
generators (they live in `stage_subitizing_draw.c`, which needs a real
viewport/platform backend to link, so they're not covered by the host-only
`make test` suite — the pure pool/step logic they depend on is covered).

**Validated on real hardware** (2026-08-26): deployed to the actual Pi
(`/opt/math-game`), built clean with the system `cc`, ran under
`math-game.service`. Progress persisted and the mastery gate correctly
advanced from sub-phase 0 to (a) after real keyboard play. One bug was
found and fixed during this: `subitize_compute_layout()`/`subitize_draw_layout()`
replaced the original single `subitize_draw_fact()` because the latter
re-randomized dot jitter (and, for counting, the entire scattered layout)
on *every* redraw — which happens on every keystroke while typing an
answer, not just once per problem — so the dots visibly jumped around as
the child typed. The fix computes the layout once when a problem is chosen
and caches it; redraws (e.g. from keystrokes) now just replay the cached
positions. Separately (not a code issue): manually running the binary
outside of `math-game.service` requires also stopping
`getty@tty1.service` yourself first — the service's `Conflicts=` relation
normally does this automatically, but a manually-launched process doesn't
get that for free, and leaving getty running fights the game for the tty.

---

## Stage 2: Number Bonds to 10 & Single-Digit Addition Strategies

**Goal**: Teach addition as a set of explicit, named strategies rather than
undifferentiated random-digit drill, per Common Core's OA progression.

**Scope**, introduced in order and interleaved once each is unlocked:
0. **Small sums** (plain `a + b = ?`, both addends 1-5, no strategy
   framing) — added after real play-testing showed starting cold on an
   unknown-addend problem (see step 1) was a harder on-ramp into symbolic
   addition than a plain small sum, even right after Stage 1. Not in the
   original research-derived scope, but consistent with it: this is
   essentially the "direct modeling" precursor Carpenter et al.'s
   progression describes before counting-on, which the original plan had
   skipped on the (evidently wrong, for a cold start) assumption that solid
   subitizing was a sufficient bridge straight into strategy-named addition.
1. **Number bonds to 10** ("3 and __ make 10") — UK EYFS ELG target.
2. **Counting-on** (start from the larger addend) — visualized via a simple
   number-line drawn with `vdraw_thick_line` plus tick marks.
3. **Doubles / near-doubles** (n+n, n+n±1).
4. **Make-ten / bridging-to-ten** (8+5 = 8+2+3) — visualized with a ten-frame
   (2×5 grid of cells via `vfill_rect`), the standard tool for this strategy.
- Operand range: single digits 0-9, sums up to 18 (covers "single-digit
  addition up to 9" and Grade 1's "within 20").

**Success Criteria**: mastery achieved across all four interleaved
sub-strategies before Stage 3 unlocks.

**Tests**: per-strategy generator correctness (e.g. make-ten items always
have an addend requiring bridging, not trivially already at a bond-of-10);
interleaving test (no long same-strategy run in the scheduled sequence).

**Status**: Complete for the scope below. Built in `stage_addition.c`
(pure step/pool logic, host-testable) and `stage_addition_draw.c`
(number-line and ten-frame rendering, depends on `viewport.h`), using the
same cumulative/interleaved pool pattern as Stage 1 — this is the second
content stage, so `game.c` now dispatches between Stage 1 and Stage 2 with
a plain `if (curriculum_stage == STAGE_SUBITIZING) ... else ...` at the
handful of points where their content actually differs (problem selection,
answer check, rendering), rather than a generic plugin/dispatch-table
abstraction — with exactly two stages, that's the boring, obvious choice;
worth revisiting into a real table only once a third stage makes the
branching unwieldy. Reaching Stage 1's `STEP_DONE` now transitions straight
into Stage 2 instead of drilling Stage 1 forever (that dead-end only
existed because Stage 2 didn't yet).

Fact counts: 25 small sums (a,b=1..5), 9 bond pairs (a=1..9, b=10-a), 12
counting-on pairs (smaller addend 1-3, larger 6-9), 9 doubles (n=1..9), 8
make-ten pairs (a=6..9, two bridging values each) — 63 facts total, all
sums ≤18. `curriculum.h`'s `SCHED_MAX_FACTS` was bumped from 32 to 80 to
fit (48 after the first bump, then to 80 for the small-sums addition).
Corrective feedback on a wrong answer shows the full solved equation (e.g.
"6 + 4 = 10" for a bond, "7 + 5 = 12" otherwise), matching Stage 0's
original design intent more literally than Stage 1's bare "It was N" (fine
there since there's only one number in play).

Anyone already partway through Stage 2 when this shipped has their
addition progress reset to the new first step (small sums) — the
mismatched fact count against their saved `progress.dat` trips Stage 0's
existing corrupted-save fallback, which was a happy accident here rather
than a deliberate migration. Their Stage 1 progress is untouched (it's
saved under a separate `stage` value).

Also fixed in this pass, from real device feedback: the mastery/step-
transition message (`draw_centered(300, 70, ...)`) used a fixed large font
size regardless of message length, so longer messages (e.g. "You're a
counting star! Let's learn addition!", measured ~1728 virtual-units wide
against the 1000-wide canvas) ran off both edges of the screen. Replaced
with `draw_centered_fit`, which shrinks the font size to keep the text
within a fixed max width instead of every caller having to hand-tune a
safe size per message. `MASTERY_MSG_MS` was also bumped from 2200ms to
3200ms per feedback that the message needed more time on screen.

One more layout fix from real device feedback: `PROMPT_Y`/`ANS_Y` (380/460)
were tuned for content with a diagram above it (Stage 1's dots, or
counting-on/make-ten's diagrams) filling the top of the screen - reused
as-is for a diagram-less addition fact (small sums, bonds, doubles), the
equation text was stranded near the bottom with the whole upper half of the
screen blank ("only occupying half the screen at bottom"). Added
`addition_fact_has_diagram()` and, in `game.c`, layout constants that
vertically center the prompt+answer block in the play area when there's no
diagram above it, rather than anchoring to the diagram-shaped layout
unconditionally.

Also enlarged per feedback that the addition equation looked too small:
`ADD_EQ_HEIGHT` (100, vs. Stage 1's `PROMPT_HEIGHT` of 70) is now addition's
own font size, verified against actual measured glyph widths for the
longest equations ("9 + 9 = 18" tops out at 588 of the 1000-wide canvas at
this size — comfortable margin, unlike the earlier mastery-message bug
where message length varied too much for any fixed size to be safe here).
The vertical gap between equation and answer (`ADD_EQ_ANS_GAP`, 120) was
widened accordingly so the taller glyphs can't run down into the answer
line below them.

Scope trimmed from the description above: near-doubles (n+n±1) aren't
implemented, only plain doubles (n+n) — near-doubles would need its own
fact-generation and answer-checking nuance (which near-double, ±1 in which
direction) that didn't seem worth the complexity before this stage has even
been played by a real child; add it as a fifth step if plain doubles proves
insufficient in practice.

---

## Stage 3: Double-Digit Addition with Regrouping

**Goal**: Extend addition to two-digit operands using place-value/base-ten
reasoning before the standard algorithm, per Common Core 2.NBT.5-6.

**Scope**:
- Operand range: two-digit + two-digit (and two-digit + one-digit), with and
  without regrouping.
- Visual scaffold: base-ten blocks (tens = tall thin rects, ones = small
  squares) shown for the first problems of the stage, then faded/optional as
  mastery within the stage increases (CPA: concrete/pictorial before
  symbolic-only) — on the physical device, this is now reinforcement of what
  the child is already doing concretely with tens/ones NFC tiles (see
  "Physical Hardware Platform"), not the only concrete representation.
- Operand pairs are chosen so sums stay within 100 (matching both the
  two-NFC-pad hardware ceiling and Common Core 2.NBT's own "within 100"
  framing for this exact content) — e.g. 45+38 is in scope, 99+99 is not.

**Success Criteria**: mastery across regrouping and non-regrouping items
before Stage 4 unlocks; visual scaffold is present early in the stage and
absent by the mastery check (so mastery reflects symbolic competence).

**Tests**: base-ten block count matches operand place values; boundary cases
within range (45+38=83, 45+55=100); regression test that Stage 2 facts still
interleave in occasionally (spaced retrieval, not abandoned once passed).

**Status**: Not Started

---

## Stage 4: Subtraction

**Goal**: Teach subtraction via both conceptual models research shows are
under-taught in combination, and connect it explicitly to addition.

**Scope**, in order:
1. **Take-away** (start with a set, remove some, count remainder) — dots
   fade/cross out from a starting group.
2. **Counting-back**.
3. **Difference/comparison** ("how many more does 7 have than 4") — two
   aligned rows of dots showing the gap, explicitly per Selter et al.'s
   finding that this model is neglected relative to take-away.
4. **Missing-addend / inverse-of-addition** (e.g. `6 + __ = 9`), tying
   directly back to Stage 2's fact families.
- Range: within 20 first (mirrors Stage 2's addition facts as the same fact
  family), then two-digit with regrouping mirroring Stage 3.

**Success Criteria**: mastery across all four sub-models, interleaved, at
each range (within-20, then two-digit) before Stage 5 unlocks.

**Tests**: correctness of each visual model's dot/removal rendering; fact-
family consistency check (a missing-addend item's answer matches the
corresponding addition fact from Stage 2's pool).

**Status**: Not Started

---

## Stage 5: Multiplication

**Goal**: Build multiplicative reasoning through the equal-groups →
skip-counting → arrays progression, arrays being the representation that
unifies the first two.

**Scope**:
1. **Equal groups / repeated addition** (e.g. 3 groups of 4, shown as 3
   clusters of 4 dots).
2. **Skip counting** (count by 4s: 4, 8, 12...).
3. **Arrays** (rows × columns grid via a `vfill_rect` grid helper) —
   introduces commutativity visually (3×4 grid = 4×3 grid rotated).
- Facts scope: operands 0-9 (Grade 3 single-digit fluency target; keeps
  results ≤81, within the 2-digit NFC place-value pad ceiling).

**Success Criteria**: mastery across all three representations, interleaved,
before Stage 6 unlocks.

**Tests**: array generator produces correct row/column counts; skip-count
sequence generator correctness; commutativity spot-check (3×4 and 4×3 both
appear across the item pool).

**Status**: Not Started

---

## Stage 6: Division

**Goal**: Teach division via both models, kept visually distinct since
research shows they are not interchangeable for young children.

**Scope**:
1. **Partitive/sharing** first (share N items into G groups — "how many
   each?" — visualized by dealing dots one at a time into G bins). Matches
   children's earliest informal fair-share intuition (~age 3.5).
2. **Quotitive/grouping** second (N items, G per group — "how many groups?"
   — visualized by bundling a pile into groups of size G).
- Scope: exact-division facts only, derived from the Stage 5 multiplication
  fact pool (no remainders — out of scope per the Non-goals section).

**Success Criteria**: mastery across both models, interleaved, completing the
curriculum sequence.

**Tests**: sharing generator always divides evenly (N = G × k); grouping
generator correctness; cross-check that every division item has a
corresponding mastered Stage 5 multiplication fact.

**Status**: Not Started

---

## Sources

Gelman & Gallistel, *The Child's Understanding of Number* (1978) — counting
principles. Clements & Sarama, *Learning and Teaching Early Math: The
Learning Trajectories Approach*; learningtrajectories.org "Perceptual
Subitizer to 4" level — subitizing/counting trajectories, including the
age-4 entry floor. UK Birth to 5 Matters (non-statutory EYFS guidance,
ages 3-4), Mathematics area of learning — age-4 number/counting targets
below the Reception ELG. Jordan,
Kaplan, Locuniak et al., several 2006-2007 studies (*Learning Disabilities
Research & Practice*; kindergarten number-sense growth studies) — predictive
validity of early number sense. Carpenter et al., *Children's Mathematical
Thinking* program; Common Core "Progressions for the Common Core State
Standards in Mathematics" (K Counting & Cardinality; K-5 OA; K-5 NBT) —
addition/subtraction/multiplication/division progressions and grade
benchmarks. Selter, Prediger, Nührenbörger & Hußmann, "Taking away and
determining the difference," *Educational Studies in Mathematics* (2012);
Carpenter & Moser, *JRME* (1984) — subtraction models. Correa, Nunes &
Bryant, "From sharing to dividing" — division models. UK DfE, *Statutory
Framework for the EYFS* (2021) and National Curriculum in England:
Mathematics — UK grade-level anchors. Leong, Ho & Cheng, "Concrete-Pictorial-
Abstract: Surveying its origins and charting its future" (NIE Singapore);
Bruner, *Toward a Theory of Instruction* (1966) — CPA. Sweller; Paas & van
Merriënboer, "Cognitive-Load Theory," *Current Directions in Psychological
Science* (2020) — cognitive load / worked examples. Bjork & Bjork,
"Introducing Desirable Difficulties Into Practice and Instruction" — spaced
retrieval. Rohrer & Taylor (2006/2010); Rohrer, Dedrick & Stershic, *Journal
of Educational Psychology* (2015) — interleaved practice. Boaler, "Timed
Tests and the Development of Math Anxiety," *Education Week* (2012), citing
Beilock's stress/working-memory research — against timed pressure for young
children. "How Does Constructive Feedback in an Educational Game Sound to
Children?", *Computers and Education: AI* (2023) — feedback design. RCTs on
adaptive/mastery-based math games, *Journal of Research on Educational
Effectiveness* (2021) and *Early Childhood Education Journal* (2022) —
mastery-based progression efficacy.

**Physical hardware platform sources**: Dujić & Granić, "Tangible interfaces
in early years' education: a systematic review," *Personal and Ubiquitous
Computing* (2021/2022); tangible-vs-touchscreen counting-app study
(ScienceDirect, "Owlet" math program) — TUIs vs. screen-only for young
children. Antle & Wise, "Getting down to details: Using learning theory to
inform tangibles research and design for children" (2013) — space-for-action
and perceptual-mapping design themes. Resnick, "Digital Manipulatives,"
*Educational Technology Research & Development* (1998); Papert on
constructionism — theoretical grounding for computationally-augmented
physical objects. Sánchez, Cortés, Riekki & Oja, NFC-based interactive
learning environments for children, ACM IDC (2011) — direct academic prior
art for NFC-tag + numeracy. Developmental memory research on
recognition-vs-recall in young children (aggregated PMC/ScienceDirect
findings) — recognition/selection tasks are far more reliable measures of
young children's competence than free recall, supporting tile-selection over
typed/recalled digit entry. Color-psychology literature on red's
failure/shame association vs. green/blue's calming association in
educational contexts (aggregated ERIC/Creativity Studies findings) — LED
color choices. Nielsen Norman Group, "Design for Kids Based on Their Stage
of Physical Development" and "UX Design for Children (Ages 3-12)" — minimum
~2cm×2cm touch/button target size and spacing for ages 3-5. AbleNet
assistive-switch specifications — low-actuation-force reference point for a
young child's Confirm button.
