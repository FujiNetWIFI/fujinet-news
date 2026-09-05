; wrap.inc -- greedy whole-word wrap for headline/title text.
;
; A transcription of the Intellivision client's wrap.bas, reading its
; source out of the reply window (RXCUR) instead of a sanitized RAM
; buffer. The server pre-wraps article BODIES to ps=20x8, but list
; headlines and the reader title arrive as one long field.
;
; Contract (wrap.bas's, with the cursor convention of parse.inc):
;   HL  IN/OUT: reply offset of the field; returns past its terminating
;       LF (or at the reply's end).
;   DE  IN: destination base. Output is (A) rows of stride 21 -- up to
;       20 characters + NUL each, ready for TXTAT.
;   A   IN: row budget (3 for headlines, 2 for the reader title).
; Text beyond the budget is dropped and the last row ellipsized ("...").
; Unused rows get a NUL at offset 0 so stale cache content never shows.
; Words longer than 20 characters (rare: URLs) hard-split at column 20.

WRAPRX: LD      (W_ROWS),A
        LD      (W_DST),DE
        PUSH    HL
        XOR     A
        LD      (W_ROW),A
        LD      (W_COL),A
        LD      A,(W_ROWS)
        LD      B,A
        LD      H,D
        LD      L,E
        LD      DE,21
WRX0:   LD      (HL),0
        ADD     HL,DE
        DJNZ    WRX0
        POP     HL

WRWORD: CALL    RXCUR           ; skip inter-word spaces
        CP      ' '
        JR      NZ,WRSK1
        INC     HL
        JR      WRWORD
WRSK1:  OR      A
        JP      Z,WRDONE        ; end of the reply
        CP      0AH
        JP      Z,WRDONE2       ; end of the field

        PUSH    HL              ; measure the word
        LD      C,0
WRMEA:  CALL    RXCUR
        OR      A
        JR      Z,WRMDN
        CP      0AH
        JR      Z,WRMDN
        CP      ' '
        JR      Z,WRMDN
        INC     HL
        INC     C
        JR      WRMEA
WRMDN:  POP     HL
        LD      A,C
        LD      (W_LEN),A

        ; A word at column 0 always starts in place; otherwise it needs
        ; a separator space too, and a whole word that would not fit
        ; closes the row instead.
        LD      A,(W_COL)
        OR      A
        JR      Z,WRCPY
        ADD     A,C
        INC     A               ; col + 1 + len
        CP      21
        JR      C,WRSEP
        CALL    WRDPTR          ; close this row
        XOR     A
        LD      (DE),A
        CALL    WRNROW
        JP      Z,WROVF
        JR      WRCPY
WRSEP:  CALL    WRDPTR
        LD      A,' '
        LD      (DE),A
        LD      A,(W_COL)
        INC     A
        LD      (W_COL),A

WRCPY:  LD      A,(W_LEN)       ; copy the word, hard-splitting at 20
        LD      B,A
WRCP1:  LD      A,(W_COL)
        CP      20
        JR      C,WRCP2
        CALL    WRDPTR          ; column 20: close and continue below
        XOR     A
        LD      (DE),A
        CALL    WRNROW
        JP      Z,WROVF
WRCP2:  CALL    RXCUR
        CALL    WRDPTR
        LD      (DE),A
        INC     HL
        LD      A,(W_COL)
        INC     A
        LD      (W_COL),A
        DJNZ    WRCP1
        JP      WRWORD

WRDONE2: INC    HL              ; consume the field's LF
WRDONE: LD      A,(W_ROWS)      ; terminate the open row
        LD      B,A
        LD      A,(W_ROW)
        CP      B
        RET     NC
        CALL    WRDPTR
        XOR     A
        LD      (DE),A
        RET

; Out of rows with text left over: ellipsize the last row -- the dots go
; right after its text (a row closed by a word that didn't fit ends
; early), clamped to column 17 so they fit before the stride boundary --
; then advance the cursor past the rest of the field.
WROVF:  LD      A,(W_ROWS)
        DEC     A
        LD      (W_ROW),A
        XOR     A
        LD      (W_COL),A
WROV1:  CALL    WRDPTR
        LD      A,(DE)
        OR      A
        JR      Z,WROV2
        LD      A,(W_COL)
        CP      17
        JR      NC,WROV2
        INC     A
        LD      (W_COL),A
        JR      WROV1
WROV2:  LD      A,'.'
        LD      (DE),A
        INC     DE
        LD      (DE),A
        INC     DE
        LD      (DE),A
        INC     DE
        XOR     A
        LD      (DE),A
WROV3:  CALL    RXCUR
        OR      A
        RET     Z
        CP      0AH
        JR      Z,WROV4
        INC     HL
        JR      WROV3
WROV4:  INC     HL
        RET

; WRDPTR: DE = (W_DST) + (W_ROW)*21 + (W_COL). Preserves A, BC, HL.
WRDPTR: PUSH    HL
        PUSH    AF
        PUSH    BC
        LD      A,(W_ROW)
        LD      L,A
        LD      H,0
        LD      E,L
        LD      D,H
        ADD     HL,HL           ; 2r
        ADD     HL,HL           ; 4r
        ADD     HL,DE           ; 5r
        ADD     HL,HL           ; 10r
        ADD     HL,HL           ; 20r
        ADD     HL,DE           ; 21r
        LD      A,(W_COL)
        LD      E,A
        LD      D,0
        ADD     HL,DE
        LD      DE,(W_DST)
        ADD     HL,DE
        EX      DE,HL
        POP     BC
        POP     AF
        POP     HL
        RET

; WRNROW: advance to the next output row. Z set when the budget is
; exhausted. Preserves HL, BC, DE.
WRNROW: PUSH    HL
        LD      HL,W_ROW
        INC     (HL)
        LD      A,(W_ROWS)
        CP      (HL)
        LD      HL,W_COL
        LD      (HL),0
        POP     HL
        RET
