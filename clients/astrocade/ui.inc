; ui.inc -- screen scaffolding shared by every state.
;
; Eleven 8-scanline rows of 20 byte-aligned 5x7 cells. Bands (header,
; footer, selection bars) are ROWFIL fills of a solid pixel byte; text
; drawn over them with a band color spec has the same background code in
; every pixel, so fill and glyphs are indistinguishable. Reply text is
; drawn straight out of the cart's reply window (DRWRXW/DRWRXL) -- one
; line staged nowhere, space-padded to full width so redraws are opaque.

; MSET: fill BC bytes at HL with A. Clobbers everything.
MSET:   LD      (HL),A
        DEC     BC
        LD      A,B
        OR      C
        RET     Z
        LD      D,H
        LD      E,L
        INC     DE
        LDIR
        RET

; CLRALL: the whole screen to color 0.
CLRALL: LD      HL,NORMEM
        LD      BC,LINES*BYTEPL
        XOR     A
        JP      MSET

; ROWADR: HL = screen address of text row B. Preserves BC, DE.
ROWADR: PUSH    DE
        LD      L,B
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; row * 64
        LD      D,B
        LD      E,0             ; row * 256
        ADD     HL,DE
        LD      DE,NORMEM
        ADD     HL,DE
        POP     DE
        RET

; ROWFIL: fill text row B with pixel byte A (00H color 0, 55H color 1,
; 0AAH color 2). Clobbers everything.
ROWFIL: PUSH    AF
        CALL    ROWADR
        POP     AF
        LD      BC,ROWBYT
        JP      MSET

; SETPAL: load the 8-byte palette table at HL -- ports 7 down to 0, four
; bytes per half in the order color 3, 2, 1, 0 (COLSET's layout).
; Clobbers A,B,C,HL.
SETPAL: LD      BC,0807H        ; B = 8 ports, C = the first (COL3L)
SPAL1:  LD      A,(HL)
        OUT     (C),A
        INC     HL
        DEC     C
        DJNZ    SPAL1
        RET

; TXTAT: DRWSTR addressed by text row -- string at HL, row D, column E,
; color spec C, at most B characters. Clobbers everything.
TXTAT:  LD      A,D
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      D,A
        JP      DRWSTR

; DRWRXW: draw up to B cells of the current reply-window line at text
; row D, column E, color spec C, starting at offset HL. Control bytes
; render as spaces; when the line ends inside the width (LF consumed, or
; the reply ran out) the remaining cells are space-filled and carry comes
; back SET. Carry clear means the width ran out first, with HL at the
; next undrawn byte, so a second call continues the same line.
; Clobbers everything.
DRWRXW: LD      A,D
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      D,A
DRXL:   CALL    RXCUR
        OR      A
        JR      Z,DRXPAD
        CP      0AH
        JR      Z,DRXLF
        INC     HL
        CP      ' '
        JR      NC,DRX2
        LD      A,' '
DRX2:   PUSH    HL
        PUSH    BC
        PUSH    DE
        CALL    DRWCH
        POP     DE
        POP     BC
        POP     HL
        INC     E
        DJNZ    DRXL
        OR      A               ; width exhausted: carry clear
        RET
DRXLF:  INC     HL              ; consume the LF
DRXPAD: LD      A,' '
        PUSH    HL
        PUSH    BC
        PUSH    DE
        CALL    DRWCH
        POP     DE
        POP     BC
        POP     HL
        INC     E
        DJNZ    DRXPAD
        SCF
        RET

; DRWRXL: as DRWRXW, then skip whatever remains of the line, leaving HL
; at the start of the next one. Clobbers everything.
DRWRXL: CALL    DRWRXW
        RET     C
DRXS1:  CALL    RXCUR
        OR      A
        RET     Z
        CP      0AH
        JR      Z,DRXS2
        INC     HL
        JR      DRXS1
DRXS2:  INC     HL
        RET

; STRLEN: C = length of the NUL-terminated string at HL. Preserves HL.
STRLEN: PUSH    HL
        LD      C,0
STRL1:  LD      A,(HL)
        OR      A
        JR      Z,STRL2
        INC     HL
        INC     C
        JR      STRL1
STRL2:  POP     HL
        RET

; STRCPY: copy the NUL-terminated string at HL to DE, stopping before
; the NUL; DE is left just past the last character. Clobbers A,HL.
STRCPY: LD      A,(HL)
        OR      A
        RET     Z
        LD      (DE),A
        INC     HL
        INC     DE
        JR      STRCPY

; DIV10: HL = HL / 10, A = the remainder. Clobbers B.
DIV10:  XOR     A
        LD      B,16
DIV1:   ADD     HL,HL
        RLA
        CP      10
        JR      C,DIV2
        SUB     10
        INC     L
DIV2:   DJNZ    DIV1
        RET

; FMTU16: render HL as unsigned decimal ASCII, NUL-terminated, built
; backward in V_NUM; returns HL -> the first digit. Clobbers A,B,D,E.
FMTU16: LD      DE,V_NUM+5
        XOR     A
        LD      (DE),A
FMT1:   CALL    DIV10
        ADD     A,'0'
        DEC     DE
        LD      (DE),A
        LD      A,H
        OR      L
        JR      NZ,FMT1
        EX      DE,HL
        RET

; PGBLD: V_PAGE = "<HL>/<DE>", NUL-terminated. Clobbers everything.
PGBLD:  PUSH    DE
        CALL    FMTU16
        LD      DE,V_PAGE
        CALL    STRCPY
        LD      A,'/'
        LD      (DE),A
        INC     DE
        POP     HL
        PUSH    DE
        CALL    FMTU16
        POP     DE
        CALL    STRCPY
        XOR     A
        LD      (DE),A
        RET

; MUL14: HL = A * 14 -- the category tables' stride. Clobbers D,E.
MUL14:  LD      L,A
        LD      H,0
        ADD     HL,HL           ; 2A
        LD      E,L
        LD      D,H
        ADD     HL,HL           ; 4A
        ADD     HL,HL           ; 8A
        ADD     HL,DE           ; 10A
        ADD     HL,DE           ; 12A
        ADD     HL,DE           ; 14A
        RET

; RECPTR: HL = V_REC + A * 80 -- list record slot A's base. Clobbers D,E.
RECPTR: LD      L,A
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; 16A
        LD      E,L
        LD      D,H
        ADD     HL,HL           ; 32A
        ADD     HL,HL           ; 64A
        ADD     HL,DE           ; 80A
        LD      DE,V_REC
        ADD     HL,DE
        RET

; FETCHUI: the fetch interstitial.
FETCHUI: LD     HL,PALSYS
        CALL    SETPAL
        CALL    CLRALL
        LD      HL,SFETCH
        LD      D,5
        LD      E,4
        LD      C,CSNORM
        LD      B,20
        JP      TXTAT

; ERRSHOW: the error screen for the current FERR, waiting for a key.
; FERR=2 shows the server's own ERROR: line (still at reply offset 0)
; across two rows; FERR=1 a fixed transport message.
ERRSHOW: LD     HL,PALSYS
        CALL    SETPAL
        CALL    CLRALL
        LD      A,(FERR)
        CP      2
        JR      NZ,ERRS1
        LD      HL,0
        LD      D,3
        LD      E,0
        LD      C,CSLBL
        LD      B,20
        CALL    DRWRXW
        JR      C,ERRS2
        LD      D,4
        LD      E,0
        LD      C,CSLBL
        LD      B,20
        CALL    DRWRXL
        JR      ERRS2
ERRS1:  LD      HL,SFAIL
        LD      D,4
        LD      E,4
        LD      C,CSLBL
        LD      B,20
        CALL    TXTAT
ERRS2:  LD      HL,SANYK
        LD      D,8
        LD      E,6
        LD      C,CSNORM
        LD      B,20
        CALL    TXTAT
        JP      INWAIT
