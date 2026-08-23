# FujiNet News for Intellivision

A three-screen reader for the 8bitnews service at `fujinet.online`,
written in IntyBASIC, talking to FujiNet through the memory-mapped
mailbox at `$9C00` (PiRTO II / Minty-FujiNet cartridge, or jzIntv's
`--fujinet` peripheral).

## Screens

| Screen | Background | What's on it |
|---|---|---|
| Topics | blue, dark-green bar | the 9 news categories, keypad 1-9 direct select |
| Article list | black, blue bar | 3 headlines per page, word-wrapped to 3 rows each |
| Reader | tan "paper", blue title band | article body as the server wrapped it (`ps=20x8`) |

Menus color-code the keys: keypad/disc tokens in yellow (red on the
reader), the action they perform in white (black on the reader).

## Controls

| Screen | Input | Action |
|---|---|---|
| Topics | disc up/down | move the bar |
| | keypad 1-9 | open that category directly |
| | Enter / action button | open the highlighted category |
| List | disc up/down | move the bar (past the edge = previous/next page) |
| | disc left/right | previous / next page |
| | keypad 1-9 | start goto-page entry: type digits, Enter jumps, Clear cancels |
| | Enter / action button | read the highlighted article |
| | Clear | back to topics |
| Reader | disc down/right / Enter | next page |
| | disc up/left | previous page |
| | keypad 0 | date + source overlay (any input returns) |
| | Clear | back to the list (no refetch) |

## Building

Requires [IntyBASIC](https://github.com/nanochess/IntyBASIC) v1.4.2 and
`as1600` from the jzIntv SDK.

```sh
make                          # produces news.bin (+.cfg) and news.rom
make INTYBASIC=/path/to/intybasic LIBDIR=/path/to/IntyBASIC/intybasic
```

Only `news.bas` is passed to the compiler; the other `.bas` files are
pulled in with `INCLUDE`.

## Running / testing

On real hardware: load `news.rom` (or `news.bin` + `news.cfg`) on a
PiRTO II with the Minty-FujiNet cartridge firmware.

Under emulation: a FujiNet-patched jzIntv plus a fujinet-firmware
instance reachable over BoIP:

```sh
./run.sh                                  # defaults: jzIntv at
                                          # ~/Workspace/jzintv-20200712-src,
                                          # FujiNet at localhost:9995
JZINTV_DIR=... FUJINET_TARGET=host:port ./run.sh
```

## Implementation notes

- `fujinet.bas` is the shared mailbox transport, copied verbatim from
  `netcat/intv` -- do not edit it here. Its header documents the
  load-bearing gotchas (MEMATTR must stop at `$9BFF`, SEQ derives from
  ACKSEQ, the v1.4.2 `AND 255` codegen bug).
- The server does the article-body word wrap and pagination
  (`ps=20x8` in the URL). The client word-wraps only list headlines
  (3 rows) and the reader title (2 rows) -- `wrap.bas`, greedy
  whole-word wrap with ellipsis on overflow.
- Article ids are kept as ASCII digits and copied into the article URL
  verbatim; ids larger than 16 bits never touch IntyBASIC arithmetic.
- Backgrounds and the selection bar use color-stack mode: each screen
  loads `MODE 0,bg,bar,bg,bar` and the bar is two `$2000` advance bits
  (first card of the region, first card after it). Bits are applied
  last in every draw path because all the text helpers write whole
  BACKTAB words.
- Scratch cart RAM map ($9000-$97FF convention; `$9100-$917F` belongs
  to fujinet.bas): response buffer `$9200` (1536 bytes), list records
  `$9800` (3 x 100), wrapped title `$9930`, number/page scratch
  `$9960`/`$9970`.

## Server API

```
https://fujinet.online/8bitnews/news.php
  ?t=lf&ps=20x8            fixed prefix (LF line endings, 20x8 wrap)
  &l=3&p=<page>&c=<name>   article list (page/total, then id|date|title)
  &p=<page>&a=<id>         one article (title, date, source, page/total, body)
```
