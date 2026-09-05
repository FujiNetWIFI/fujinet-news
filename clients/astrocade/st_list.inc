; st_list.inc -- article-list screen.
;
; White on black with blue bands and a dark-magenta selection bar
; (PALLST), the Intellivision client's scheme. Header row 0: category
; title left, cur/max right-aligned. Three articles per page (server
; l=3), each headline word-wrapped client-side across rows 1+3i..3+3i.
; Footer row 10. Up past the top goes to the previous page with the
; cursor at the bottom; down past the bottom to the next with it at the
; top (the coco behavior). Any keypad digit 1-9 starts goto-page entry
; on the footer; trigger/= jumps, CE/C cancels.
;
; The page lives in the V_REC cache, so coming back from the reader (or
; a failed article fetch, which clobbered the reply window) redraws
; without a network round trip, and a failed page flip falls back to the
; page still cached.

LISTENT: CALL   FETCHUI
        XOR     A
        LD      (REQTYP),A
        CALL    FETCH
        LD      A,(FERR)
        OR      A
        JP      NZ,LISTFL
        CALL    LSTPRS
        LD      A,(NARTS)
        OR      A
        JR      NZ,LSTE1
        LD      A,1             ; parsed to nothing: treat as a failure
        LD      (FERR),A
        JP      LISTFL
LSTE1:  LD      A,1
        LD      (LISTOK),A
        LD      A,(LISTSEL)     ; clamp the cursor to this page
        LD      HL,NARTS
        CP      (HL)
        JR      C,LISTSHW
        LD      A,(HL)
        DEC     A
        LD      (LISTSEL),A

LISTSHW: LD     HL,PALLST
        CALL    SETPAL
        CALL    LISTDRW

LISTLP: CALL    DELAY10
        CALL    INPOLR
        OR      A
        JR      Z,LISTLP
        CP      EVUP
        JR      Z,LSUP
        CP      EVDOWN
        JR      Z,LSDN
        CP      EVLEFT
        JR      Z,LSPRV
        CP      EVRIGHT
        JR      Z,LSNXT
        CP      EVTRIG
        JP      Z,LSREAD
        CP      EVEQ
        JP      Z,LSREAD
        CP      EVCE
        JP      Z,TOPENT
        CP      EVCLR
        JP      Z,TOPENT
        CP      EVD0+1
        JR      C,LISTLP
        CP      EVD0+10
        JP      C,LSGOTO
        JR      LISTLP

LSUP:   LD      A,(LISTSEL)
        OR      A
        JR      Z,LSUPPG
        DEC     A
        JR      LSMOV
LSUPPG: LD      HL,(LPCUR)      ; up past the top: previous page,
        DEC     HL              ; cursor at the bottom
        LD      A,H
        OR      L
        JR      Z,LISTLP
        LD      (LPREQ),HL
        LD      A,2
        LD      (LISTSEL),A
        JP      LISTENT

LSDN:   LD      A,(LISTSEL)
        LD      HL,NARTS
        INC     A
        CP      (HL)
        JR      NC,LSDNPG
        JR      LSMOV
LSDNPG: CALL    LSMORE          ; down past the bottom: next page,
        JR      C,LISTLP        ; cursor at the top
        XOR     A
        LD      (LISTSEL),A
        JP      LISTENT

LSPRV:  LD      HL,(LPCUR)
        DEC     HL
        LD      A,H
        OR      L
        JR      Z,LISTLP
        LD      (LPREQ),HL
        JP      LISTENT

LSNXT:  CALL    LSMORE
        JR      C,LISTLP
        JP      LISTENT

; LSMORE: carry clear (and LPREQ set) when a next page exists.
LSMORE: LD      HL,(LPMAX)
        LD      DE,(LPCUR)
        OR      A
        SBC     HL,DE
        LD      A,H
        OR      L
        JR      Z,LSMNO
        INC     DE
        LD      (LPREQ),DE
        OR      A
        RET
LSMNO:  SCF
        RET

; LSMOV: A = the new selection; repaint the old block and the new.
LSMOV:  LD      HL,LISTSEL
        LD      D,(HL)
        LD      (HL),A
        PUSH    AF
        LD      A,D
        CALL    ARTROW
        POP     AF
        CALL    ARTROW
        JP      LISTLP

LSREAD: LD      HL,1
        LD      (APREQ),HL
        JP      RDRENT

; --- Goto-page numeric entry -------------------------------------------

LSGOTO: SUB     EVD0            ; the seed digit, 1-9
        LD      L,A
        LD      H,0
        LD      (GOTOV),HL
        LD      A,1
        LD      (GOTOL),A
        CALL    LGDRAW
LGLOOP: CALL    INWAIT
        CP      EVTRIG
        JR      Z,LGGO
        CP      EVEQ
        JR      Z,LGGO
        CP      EVCE
        JR      Z,LGCAN
        CP      EVCLR
        JR      Z,LGCAN
        CP      EVD0
        JR      C,LGLOOP
        CP      EVD0+10
        JR      NC,LGLOOP
        SUB     EVD0
        LD      C,A
        LD      A,(GOTOL)
        CP      4
        JR      NC,LGLOOP
        INC     A
        LD      (GOTOL),A
        LD      HL,(GOTOV)
        ADD     HL,HL           ; accumulator * 10 + digit
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,DE
        LD      E,C
        LD      D,0
        ADD     HL,DE
        LD      (GOTOV),HL
        CALL    LGDRAW
        JR      LGLOOP

LGGO:   LD      HL,(GOTOV)
        LD      A,H
        OR      L
        JR      NZ,LGG1
        INC     HL              ; 0 -> 1
LGG1:   LD      DE,(LPMAX)
        PUSH    HL
        OR      A
        SBC     HL,DE
        POP     HL
        JR      C,LGG2
        LD      H,D             ; clamp to the last page
        LD      L,E
LGG2:   LD      DE,(LPCUR)
        PUSH    HL
        OR      A
        SBC     HL,DE
        POP     HL
        JR      NZ,LGG3
LGCAN:  CALL    LSTFTR          ; same page / cancelled: restore footer
        JP      LISTLP
LGG3:   LD      (LPREQ),HL
        JP      LISTENT

LGDRAW: LD      B,FOOTROW
        LD      A,55H
        CALL    ROWFIL
        LD      HL,SGOTO
        LD      D,FOOTROW
        LD      E,0
        LD      C,CSBND
        LD      B,20
        CALL    TXTAT
        LD      HL,(GOTOV)
        CALL    FMTU16
        LD      D,FOOTROW
        LD      E,11
        LD      C,CSBND
        LD      B,5
        JP      TXTAT

; --- Parse -------------------------------------------------------------

; LSTPRS: "cur/total" then up to three "id|timestamp|headline" lines.
; The id keeps its ASCII digits (V_REC slot +0); the timestamp is
; discarded (the info overlay uses the article's own pubdate); the
; headline wraps to 3 rows at slot +16. A malformed record stops the
; page. Page numbers of 0 are coerced to 1.
LSTPRS: LD      HL,0
        CALL    PU16
        LD      A,D
        OR      E
        JR      NZ,LSP1
        INC     E
LSP1:   LD      (LPCUR),DE
        CALL    RXCUR
        CP      '/'
        JR      NZ,LSP2
        INC     HL
LSP2:   CALL    PU16
        LD      A,D
        OR      E
        JR      NZ,LSP3
        INC     E
LSP3:   LD      (LPMAX),DE
        CALL    SKIPLF
        XOR     A
        LD      (NARTS),A
LSPREC: CALL    RXCUR
        OR      A
        RET     Z               ; end of the reply
        PUSH    HL
        LD      A,(NARTS)
        CALL    RECPTR
        EX      DE,HL           ; DE = the record's id field
        POP     HL
        CALL    CPYID
        RET     C
        CALL    SKPBAR          ; the timestamp
        RET     C
        PUSH    HL
        LD      A,(NARTS)
        CALL    RECPTR
        LD      DE,16
        ADD     HL,DE
        EX      DE,HL           ; DE = the headline rows
        POP     HL
        LD      A,3
        CALL    WRAPRX
        LD      A,(NARTS)
        INC     A
        LD      (NARTS),A
        CP      3
        JR      C,LSPREC
        RET

; --- Drawing -----------------------------------------------------------

LISTDRW: LD     B,0
        LD      A,55H
        CALL    ROWFIL
        LD      A,(TOPSEL)
        CALL    MUL14
        LD      DE,CATTIT
        ADD     HL,DE
        LD      D,0
        LD      E,0
        LD      C,CSBND
        LD      B,13
        CALL    TXTAT
        LD      HL,(LPCUR)
        LD      DE,(LPMAX)
        CALL    PGBLD
        LD      HL,V_PAGE
        CALL    STRLEN
        LD      A,20
        SUB     C
        LD      E,A
        LD      D,0
        LD      C,CSBND
        LD      B,12
        CALL    TXTAT
        XOR     A
LSD1:   PUSH    AF
        CALL    ARTROW
        POP     AF
        INC     A
        CP      3
        JR      C,LSD1
LSTFTR: LD      B,FOOTROW       ; LISTDRW's tail doubles as the footer
        LD      A,55H           ; restorer for goto-page entry
        CALL    ROWFIL
        LD      HL,SLSTFT
        LD      D,FOOTROW
        LD      E,0
        LD      C,CSBND
        LD      B,20
        JP      TXTAT

; ARTROW: draw article slot A's three rows -- magenta bar + white when
; selected, plain when not, blanked past NARTS. Clobbers everything.
ARTROW: LD      (SCRA),A
        LD      HL,LISTSEL
        SUB     (HL)
        LD      (SCRB),A        ; 0 = selected
        XOR     A
        LD      (SCRC),A        ; headline row 0-2
ARTR1:  LD      A,(SCRA)        ; text row = 1 + slot*3 + line
        LD      B,A
        ADD     A,A
        ADD     A,B
        INC     A
        LD      HL,SCRC
        ADD     A,(HL)
        LD      B,A
        LD      A,(SCRA)
        LD      HL,NARTS
        CP      (HL)
        JR      NC,ARTRE        ; an empty slot: just blank the row
        LD      A,(SCRB)
        OR      A
        LD      A,0AAH          ; selected: the magenta bar
        JR      Z,ARTR2
        XOR     A
ARTR2:  CALL    ROWFIL
        LD      A,(SCRA)        ; the cached headline row
        CALL    RECPTR
        LD      DE,16
        ADD     HL,DE
        LD      A,(SCRC)
        OR      A
        JR      Z,ARTR3
        LD      DE,21
ARTR2A: ADD     HL,DE
        DEC     A
        JR      NZ,ARTR2A
ARTR3:  LD      A,(SCRA)
        LD      B,A
        ADD     A,A
        ADD     A,B
        INC     A
        LD      B,A
        LD      A,(SCRC)
        ADD     A,B
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      D,A             ; y scanline
        LD      E,0
        LD      A,(SCRB)
        OR      A
        LD      C,CSSEL
        JR      Z,ARTR4
        LD      C,CSNORM
ARTR4:  LD      B,20
        CALL    DRWSTR
        JR      ARTRN
ARTRE:  XOR     A
        CALL    ROWFIL
ARTRN:  LD      HL,SCRC
        INC     (HL)
        LD      A,(HL)
        CP      3
        JR      C,ARTR1
        RET

; --- Failure -----------------------------------------------------------

LISTFL: CALL    ERRSHOW
        LD      A,(LISTOK)
        OR      A
        JP      Z,TOPENT        ; nothing cached yet: back to topics
        LD      HL,(LPCUR)      ; a failed flip: fall back to the page
        LD      (LPREQ),HL      ; still in the cache
        JP      LISTSHW
