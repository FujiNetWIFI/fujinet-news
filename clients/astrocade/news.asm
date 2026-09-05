; news.asm -- FujiNet News for the Bally Astrocade.
;
; A port of the Intellivision client (fujinet-news/clients/intv): boot ->
; topics -> article list -> reader -> info overlay, over the same
; news.php protocol. The server does the body wrap and pagination
; (ps=20x8); the client wraps only list headlines and the reader title.
;
; Unlike the 4x6 family (netcat, gcal), text goes through fujinet-config's
; 5x7 byte-aligned blitter (font.inc): BIOS glyphs for 20H-63H plus its
; own lowercase, cells 8 pixels wide, 20 columns -- big and readable, and
; the same column count as the Intellivision original, so the layouts port
; row for row.
;
; ROM budget: 0000H-1AFFH of the 8K window (6,912 bytes); 1B00H+ belongs
; to the mailbox and build.sh stamps the "FUJI" claim at 1CFCH.
;
; RAM is screen RAM, full stop. LINES=88 shows 11 rows of 5x7 text and
; leaves 4DC0H up -- 576 bytes -- to the program; 80 lines (config's
; choice) cannot seat header + nine categories + footer, and 96 would
; leave too little for the list-record cache below.
;
; Interrupts stay off for the program's whole life (fujilib.inc's
; contract: with I = 0, refresh strays land in OS ROM and never hit the
; hotspots).

        INCLUDE "HVGLIB.H"
        INCLUDE "fujinet.inc"

; ---- geometry ---------------------------------------------------------
LINES   EQU     88              ; visible scanlines: 11 rows of 5x7
NCOLS   EQU     20              ; 8-pixel cells, 2 bytes each at 2bpp
FOOTROW EQU     10              ; the footer row
ROWBYT  EQU     8*BYTEPL        ; one text row of screen RAM

; ---- RAM map ----------------------------------------------------------
; 4000H-4DBFH is the visible screen. Everything below is ours. The list
; page is cached in V_REC (the Intellivision's SC_REC move, less the
; timestamp it never displayed): backing out of the reader and failed
; page flips redraw the list without a network round trip.
V_REC   EQU     4DC0H           ; 240: 3 records, stride 80:
                                ;   +0  id, ASCII digits, NUL-terminated
                                ;   +16 headline wrapped 3 rows x stride 21
V_TITLE EQU     4EB0H           ; 42: reader title wrapped 2 x stride 21
V_PAGE  EQU     4EDAH           ; 12: "cur/max" page-indicator string
V_NUM   EQU     4EE6H           ; 6: FMTU16 builds digits backward here
; 4EECH-4EFFH spare (still cleared by the boot FILL)
V_KEY   EQU     4F00H           ; input: last raw key, for edge detection
V_RPT   EQU     4F01H           ; input: auto-repeat countdown
CURSLC  EQU     4F02H           ; reply slice the cart is publishing
AVAIL   EQU     4F03H           ; word: NET_STATUS bytes waiting
RXLEN   EQU     4F05H           ; word: reply length, captured after READ
PRVAVL  EQU     4F07H           ; word: the settle loop's previous reading
REQTYP  EQU     4F09H           ; 0 = list fetch, 1 = article fetch
FERR    EQU     4F0AH           ; 0 ok / 1 transport / 2 server ERROR
TOPSEL  EQU     4F0BH           ; selected topic 0-8
LISTSEL EQU     4F0CH           ; selected article 0-2
NARTS   EQU     4F0DH           ; articles on the cached page
LISTOK  EQU     4F0EH           ; nonzero once a list page is cached
LPREQ   EQU     4F0FH           ; word: list page to request
LPCUR   EQU     4F11H           ; word: cached list page, current
LPMAX   EQU     4F13H           ; word: cached list page, total
APREQ   EQU     4F15H           ; word: article page to request
APCUR   EQU     4F17H           ; word: article page, current
APMAX   EQU     4F19H           ; word: article page, total
GOTOV   EQU     4F1BH           ; word: goto-page accumulator
GOTOL   EQU     4F1DH           ; goto-page digits entered
ADATEO  EQU     4F1EH           ; word: reply offset of the pubdate field
ASRCO   EQU     4F20H           ; word: reply offset of the source field
ABODYO  EQU     4F22H           ; word: reply offset of the first body row
W_ROWS  EQU     4F24H           ; wrap.inc: row budget
W_ROW   EQU     4F25H           ; wrap.inc: current row
W_COL   EQU     4F26H           ; wrap.inc: current column
W_LEN   EQU     4F27H           ; wrap.inc: measured word length
W_DST   EQU     4F28H           ; word: wrap.inc destination base
SCRA    EQU     4F2AH           ; screen-draw scratch
SCRB    EQU     4F2BH
SCRC    EQU     4F2CH
; 4F2DH-4FBFH stack slack (call tree is shallow)
STACK   EQU     4FC0H           ; grows down; 4FC0H+ left to the BIOS cells

; ---- color specs (font.inc blitter: bits 1-0 fg, 3-2 bg) --------------
; Every screen keeps the same roles: color 0 background, color 1 band and
; selection fill, color 2 accent, color 3 text -- so one set of specs
; works under all five palettes and a palette swap restyles a screen
; without a redraw (the info overlay leans on that).
CSNORM  EQU     03H             ; color 3 on background
CSACC   EQU     02H             ; color 2 on background
CSLBL   EQU     01H             ; color 1 on background
CSBND   EQU     07H             ; color 3 on the color-1 band
CSBNDA  EQU     06H             ; color 2 on the color-1 band
CSSEL   EQU     0BH             ; color 3 on the color-2 selection bar

; MB_* labels are module fences for tools/checksize.py's budget table.
        ORG     FIRSTC
MB_MAIN:
        DB      55H             ; menued-cartridge sentinel
        DW      MENUST          ; chain to the on-board SELECT GAME list
        DW      PRGNAM
        DW      PRGSTR
PRGNAM: DB      "FUJINET NEWS"
        DB      0

PRGSTR: DI
        LD      SP,STACK
        SYSTEM  INTPC
        DO      SETOUT
        DB      LINES*2
        DB      0               ; HORCB 0: whole line on the right palette
        DB      8
        DO      COLSET
        DW      PALSYS
        DO      FILL            ; clear past the visible lines through the
        DW      NORMEM          ; variable region (4000H-4EFFH): the vars
        DW      0F00H           ; live in screen RAM above the display,
        DB      0               ; where SELECT GAME leaves stale text
        EXIT

        ; The keypress that picked us off the on-board menu is still
        ; down. Drain it, or the topics screen takes it as a selection.
KWAIT:  CALL    KEYRAW
        OR      A
        JR      NZ,KWAIT

        XOR     A
        LD      (V_KEY),A
        LD      (V_RPT),A
        LD      (CURSLC),A
        LD      (LISTOK),A
        LD      (TOPSEL),A
        LD      (LISTSEL),A

        LD      HL,SHDR
        LD      D,2
        LD      E,4
        LD      C,CSACC
        LD      B,20
        CALL    TXTAT
BOOTCK: CALL    FNCHECK
        JP      Z,TOPENT
        LD      HL,SNOFN
        LD      D,5
        LD      E,1
        LD      C,CSLBL
        LD      B,20
        CALL    TXTAT
        LD      HL,SRETRY
        LD      D,7
        LD      E,2
        LD      C,CSNORM
        LD      B,20
        CALL    TXTAT
        CALL    INWAIT
        JR      BOOTCK

; ---- Data -------------------------------------------------------------
MB_DATA:
; COLSET stores descending, ports 7 down to 0, four bytes per palette in
; the order color 3, 2, 1, 0; both halves identical since HORCB is 0.
; Byte = (hue << 3) | luminance, hue 0 the grayscale column. Each screen
; reloads its palette on entry (SETPAL) -- the Astrocade analog of the
; Intellivision client re-issuing MODE per screen. Roles per the CS*
; comment above; values tuned in MAME.
PALSYS: DB      07H,75H,4CH,00H         ; white/yellow/red on black:
        DB      07H,75H,4CH,00H         ;   boot, fetching, error
PALTOP: DB      07H,75H,0C1H,0F1H       ; white text, yellow digits,
        DB      07H,75H,0C1H,0F1H       ;   dark-green bands, deep blue
PALLST: DB      07H,21H,0F3H,00H        ; white text, magenta selection,
        DB      07H,21H,0F3H,00H        ;   blue bands, black
PALRDR: DB      00H,07H,0F2H,5FH        ; black ink, white band text,
        DB      00H,07H,0F2H,5FH        ;   blue bands, tan paper
PALINF: DB      00H,07H,51H,5FH         ; the reader with color 1 turned
        DB      00H,07H,51H,5FH         ;   red: bands flip, no redraw

SHDR:   DB      "FUJINET NEWS",0
SNOFN:  DB      "FujiNet not found",0
SRETRY: DB      "Any key retries",0
SFETCH: DB      "Fetching...",0
SFAIL:  DB      "Fetch failed",0
SANYK:  DB      "any key",0
SERRLIT: DB     "ERROR:",0
STOPFT: DB      "1-9 ^v:pick  =:read",0
SLSTFT: DB      "^v:pick <>:pg #:goto",0
SRDFT:  DB      "^v:pg 0:info",0
SGOTO:  DB      "Goto page: ",0
SDATE:  DB      "Date:",0
SSRC:   DB      "Source:",0

; Wire category names for c= (lowercase, the coco client's order) and
; their display titles, both stride 14, NUL-padded.
CATNAMS: DB     "top",0,0,0,0,0,0,0,0,0,0,0
        DB      "world",0,0,0,0,0,0,0,0,0
        DB      "business",0,0,0,0,0,0
        DB      "science",0,0,0,0,0,0,0
        DB      "technology",0,0,0,0
        DB      "health",0,0,0,0,0,0,0,0
        DB      "entertainment",0
        DB      "politics",0,0,0,0,0,0
        DB      "sports",0,0,0,0,0,0,0,0
CATTIT: DB      "Top Stories",0,0,0
        DB      "World News",0,0,0,0
        DB      "Business",0,0,0,0,0,0
        DB      "Science",0,0,0,0,0,0,0
        DB      "Technology",0,0,0,0
        DB      "Health",0,0,0,0,0,0,0,0
        DB      "Entertainment",0
        DB      "Politics",0,0,0,0,0,0
        DB      "Sports",0,0,0,0,0,0,0,0

        INCLUDE "build/endpoint.inc"

MB_TOPICS:
        INCLUDE "st_topics.inc"
MB_LIST:
        INCLUDE "st_list.inc"
MB_READER:
        INCLUDE "st_reader.inc"
MB_WRAP:
        INCLUDE "wrap.inc"
MB_PARSE:
        INCLUDE "parse.inc"
MB_UI:
        INCLUDE "ui.inc"
MB_URL:
        INCLUDE "url.inc"
MB_INPUT:
        INCLUDE "input.inc"
MB_NET:
        INCLUDE "net.inc"
MB_STATE:
        INCLUDE "state.inc"
MB_FONT:
        INCLUDE "font.inc"
MB_FUJILIB:
        INCLUDE "fujilib.inc"
MB_END:
