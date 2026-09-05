; st_reader.inc -- article reader screen.
;
; Black ink on tan "paper" with blue bands (PALRDR), the Intellivision
; client's scheme. Title band rows 0-1 (client-wrapped to V_TITLE), body
; rows 2-9 drawn straight from the reply window exactly as the server
; wrapped them (ps=20x8), footer band row 10 with cur/max and hints.
; Page turns re-fetch (the server paginates). CE/C returns to the list,
; which redraws from its RAM cache -- no refetch.
;
; Keypad 0 overlays date and source. The overlay is entered with a
; palette swap (PALINF turns color 1 red, so both bands flip with no
; redraw) plus a repaint of rows 2-9 from offsets still valid in the
; window -- the overlay runs no transactions, so leaving it redraws the
; body the same way.

RDRENT: CALL    FETCHUI
        LD      A,1
        LD      (REQTYP),A
        CALL    FETCH
        LD      A,(FERR)
        OR      A
        JP      NZ,RDRFL
        CALL    ARTPRS

RDRSHW: LD      HL,PALRDR
        CALL    SETPAL
        LD      B,0             ; the title band
        LD      A,55H
        CALL    ROWFIL
        LD      B,1
        LD      A,55H
        CALL    ROWFIL
        LD      HL,V_TITLE
        LD      D,0
        LD      E,0
        LD      C,CSBNDA
        LD      B,20
        CALL    TXTAT
        LD      HL,V_TITLE+21
        LD      D,1
        LD      E,0
        LD      C,CSBNDA
        LD      B,20
        CALL    TXTAT
        CALL    RDRBDY
        LD      B,FOOTROW       ; the footer band
        LD      A,55H
        CALL    ROWFIL
        LD      HL,(APCUR)
        LD      DE,(APMAX)
        CALL    PGBLD
        LD      HL,V_PAGE
        LD      D,FOOTROW
        LD      E,0
        LD      C,CSBNDA
        LD      B,20
        CALL    TXTAT
        LD      HL,SRDFT
        LD      D,FOOTROW
        LD      E,8
        LD      C,CSBNDA
        LD      B,20
        CALL    TXTAT

RDRLP:  CALL    DELAY10
        CALL    INPOLR
        OR      A
        JR      Z,RDRLP
        CP      EVUP
        JR      Z,RDRPRV
        CP      EVLEFT
        JR      Z,RDRPRV
        CP      EVDOWN
        JR      Z,RDRNXT
        CP      EVRIGHT
        JR      Z,RDRNXT
        CP      EVTRIG
        JR      Z,RDRNXT
        CP      EVEQ
        JR      Z,RDRNXT
        CP      EVD0
        JR      Z,RDRNFO
        CP      EVCE
        JP      Z,LISTSHW
        CP      EVCLR
        JP      Z,LISTSHW
        JR      RDRLP

RDRPRV: LD      HL,(APCUR)
        DEC     HL
        LD      A,H
        OR      L
        JR      Z,RDRLP
        LD      (APREQ),HL
        JP      RDRENT
RDRNXT: LD      HL,(APMAX)
        LD      DE,(APCUR)
        OR      A
        SBC     HL,DE
        LD      A,H
        OR      L
        JR      Z,RDRLP
        INC     DE
        LD      (APREQ),DE
        JP      RDRENT

RDRFL:  CALL    ERRSHOW         ; the failed fetch clobbered the window;
        JP      LISTSHW         ; the list redraws from its RAM cache

; --- Info overlay ------------------------------------------------------

RDRNFO: LD      HL,PALINF
        CALL    SETPAL
        LD      A,2
RDNF1:  LD      (SCRC),A
        LD      B,A
        XOR     A
        CALL    ROWFIL
        LD      A,(SCRC)
        INC     A
        CP      10
        JR      C,RDNF1
        LD      HL,SDATE
        LD      D,3
        LD      E,0
        LD      C,CSLBL
        LD      B,20
        CALL    TXTAT
        LD      HL,(ADATEO)
        LD      D,4
        LD      E,0
        LD      C,CSNORM
        LD      B,20
        CALL    DRWRXL
        LD      HL,SSRC
        LD      D,6
        LD      E,0
        LD      C,CSLBL
        LD      B,20
        CALL    TXTAT
        LD      HL,(ASRCO)      ; the source gets two rows
        LD      D,7
        LD      E,0
        LD      C,CSNORM
        LD      B,20
        CALL    DRWRXW
        JR      C,RDNF2
        LD      D,8
        LD      E,0
        LD      C,CSNORM
        LD      B,20
        CALL    DRWRXL
RDNF2:  LD      HL,SANYK
        LD      D,9
        LD      E,6
        LD      C,CSLBL
        LD      B,20
        CALL    TXTAT
        CALL    INWAIT
        LD      HL,PALRDR
        CALL    SETPAL
        CALL    RDRBDY
        JP      RDRLP

; --- Parse / drawing ---------------------------------------------------

; ARTPRS: title / pubdate / source / "cur/total" / body rows. The title
; wraps to V_TITLE; date, source and body stay in the window as offsets.
ARTPRS: LD      HL,0
        LD      DE,V_TITLE
        LD      A,2
        CALL    WRAPRX
        LD      (ADATEO),HL
        CALL    SKIPLF
        LD      (ASRCO),HL
        CALL    SKIPLF
        CALL    PU16
        LD      A,D
        OR      E
        JR      NZ,ARTP1
        INC     E
ARTP1:  LD      (APCUR),DE
        CALL    RXCUR
        CP      '/'
        JR      NZ,ARTP2
        INC     HL
ARTP2:  CALL    PU16
        LD      A,D
        OR      E
        JR      NZ,ARTP3
        INC     E
ARTP3:  LD      (APMAX),DE
        CALL    SKIPLF
        LD      (ABODYO),HL
        RET

; RDRBDY: rows 2-9 straight from the window, opaque, blanking whatever
; the page didn't fill.
RDRBDY: LD      HL,(ABODYO)
        LD      A,2
RDB1:   LD      (SCRC),A
        LD      D,A
        LD      E,0
        LD      C,CSNORM
        LD      B,20
        CALL    DRWRXL
        LD      A,(SCRC)
        INC     A
        CP      10
        JR      C,RDB1
        RET
