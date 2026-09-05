# FujiNet News for the Bally Astrocade

A port of the Intellivision client (`clients/intv`): topics, article list,
reader, info overlay, over the same `news.php` protocol. Z80 assembly (zmac),
standalone like the intv client -- not wired into the mekkogx `clients/`
Makefile.

Text goes through fujinet-config's byte-aligned 5x7 blitter -- BIOS glyphs
plus its own lowercase, the character set the on-screen keyboard there draws
-- at 20 columns x 11 rows (LINES=88). Bigger and more readable than the
4x6 family, and the same column count as the Intellivision original, so the
layouts port row for row.

## Screens and colors

Each screen loads its own palette on entry (the Astrocade analog of the
Intellivision client re-issuing `MODE`), keeping the same color roles --
0 background, 1 bands/selection, 2 accent, 3 text -- so one set of blitter
color specs works everywhere:

| Screen  | Look |
|---------|------|
| topics  | white/yellow on deep blue, dark-green bands and selection bar |
| list    | white on black, blue bands, dark-magenta selection bar |
| reader  | black ink on tan paper, blue title/footer bands |
| info    | the reader with color 1 turned red -- a palette-only overlay |
| system  | white/yellow/red on black (boot, fetching, errors) |

## Controls

Keypad and hand controller, netcat's event space:

| Key | Topics | List | Reader |
|-----|--------|------|--------|
| stick up/down, keypad ^/v | move the bar | move the bar / page at the edges | page (refetch) |
| stick left/right | -- | previous/next page | page |
| trigger, = | read the topic | read the article | next page |
| 1-9 | pick directly | goto-page entry (digits, = jumps, CE cancels) | -- |
| 0 | -- | -- | date/source overlay |
| CE, C | -- | back to topics | back to the list |

The list page is cached in RAM, so backing out of the reader (and a failed
article fetch or page flip) redraws instantly with no network round trip.

## Protocol

One-shot HTTP GETs against
`N:HTTPS://FUJINET.ONLINE/8bitnews/news.php?t=lf&ps=20x8` (override with
`ENDPOINT=` at build time):

    &l=3&p=<page>&c=<category>   article list: cur/total, then id|date|title
    &p=<page>&a=<id>             one article: title, date, source, cur/total, body

The server wraps the body to `ps` and paginates; the client wraps only list
headlines and the reader title (wrap.inc). `l=3` is the largest list that
provably fits the cart's 1KB reply window, so a single READ captures every
response and the screens render straight out of the window -- nothing is
buffered except the three cached records and the wrapped title. Article ids
travel as ASCII digits verbatim (they can exceed 16 bits).

## Build

    make            # build.sh: zmac, pad to 8K, FUJI claim, layout checks
    make run        # MAME with the fujinet cart against a live fujinet-pc
    make smoke      # headless: topics -> list -> reader, with snapshots
    make clean

Needs zmac (found via PATH, `~/Workspace/zmac-1.3`, or the firmware
bring-up tree) and, to run, a MAME with the fujinet cart device applied
(`fujinet-firmware/pico/astrocade/emu/apply.sh`) plus a fujinet-pc BoIP
listener at `FUJINET_TCP` (default 127.0.0.1:9995). At the on-screen menu,
keypad **1** starts the client. `emu/tour.lua` and `emu/tour2.lua` are
fuller headless walks (every screen; goto-page and paging).

The cart serves an 8K window; the mailbox owns 1B00H up, so code and data
end below that and build.sh stamps the "FUJI" claim at 1CFCH. RAM is screen
RAM: LINES=88 leaves 576 bytes at 4DC0H-4FFFH for variables, the record
cache, and the stack (the map is in news.asm).

fujilib.inc, state.inc, input.inc and HVGLIB.H are verbatim copies from
netcat/astrocade; font.inc is fujinet-config's. Keep them in step with
their sources.
