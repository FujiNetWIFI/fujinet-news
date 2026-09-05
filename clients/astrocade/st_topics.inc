; st_topics.inc -- topic-selection screen.
;
; White/yellow on deep blue with dark-green bands (PALTOP), the
; Intellivision client's scheme. Header band row 0, nine category rows
; 1-9 (keypad digit + title), footer band row 10. The selection bar is a
; color-1 fill -- the same green as the chrome -- with both digit and
; title redrawn white on it. Keypad 1-9 selects directly; up/down + the
; trigger (or =) select by bar.

TOPENT: LD      HL,PALTOP
        CALL    SETPAL
        CALL    CLRALL
        XOR     A
        LD      (LISTOK),A      ; the record cache dies with the topic
        LD      B,0
        LD      A,55H
        CALL    ROWFIL
        LD      HL,SHDR
        LD      D,0
        LD      E,4
        LD      C,CSBNDA
        LD      B,20
        CALL    TXTAT
        XOR     A
TOPDR1: PUSH    AF
        CALL    TOPROW
        POP     AF
        INC     A
        CP      9
        JR      C,TOPDR1
        LD      B,FOOTROW
        LD      A,55H
        CALL    ROWFIL
        LD      HL,STOPFT
        LD      D,FOOTROW
        LD      E,0
        LD      C,CSBNDA
        LD      B,20
        CALL    TXTAT

TOPLP:  CALL    DELAY10
        CALL    INPOLR
        OR      A
        JR      Z,TOPLP
        CP      EVUP
        JR      Z,TOPUP
        CP      EVDOWN
        JR      Z,TOPDN
        CP      EVTRIG
        JR      Z,TOPGO
        CP      EVEQ
        JR      Z,TOPGO
        CP      EVD0+1
        JR      C,TOPLP
        CP      EVD0+10
        JR      NC,TOPLP
        SUB     EVD0+1
        LD      (TOPSEL),A      ; direct pick; the bar needn't move first
        JR      TOPGO

TOPUP:  LD      A,(TOPSEL)
        OR      A
        JR      Z,TOPLP
        DEC     A
        JR      TOPMV
TOPDN:  LD      A,(TOPSEL)
        CP      8
        JR      NC,TOPLP
        INC     A
TOPMV:  LD      HL,TOPSEL
        LD      D,(HL)          ; the old row, to unpaint
        LD      (HL),A
        PUSH    AF
        LD      A,D
        CALL    TOPROW
        POP     AF
        CALL    TOPROW
        JR      TOPLP

TOPGO:  LD      HL,1
        LD      (LPREQ),HL
        XOR     A
        LD      (LISTSEL),A
        JP      LISTENT

; TOPROW: draw category row for topic index A (0-8), in the selected
; style when A = (TOPSEL). Clobbers everything.
TOPROW: LD      (SCRA),A
        LD      HL,TOPSEL
        SUB     (HL)
        LD      (SCRB),A        ; 0 = selected
        LD      A,(SCRA)
        INC     A
        LD      B,A             ; its text row
        LD      A,(SCRB)
        OR      A
        LD      A,55H           ; selected: the green bar
        JR      Z,TPR1
        XOR     A               ; unselected: background
TPR1:   CALL    ROWFIL
        LD      A,(SCRB)        ; the keypad digit, column 1
        OR      A
        LD      C,CSBND
        JR      Z,TPR2
        LD      C,CSACC
TPR2:   LD      A,(SCRA)
        INC     A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      D,A             ; y scanline
        LD      E,1
        LD      A,(SCRA)
        ADD     A,'1'
        CALL    DRWCH
        LD      A,(SCRA)        ; the title, column 3
        CALL    MUL14
        LD      DE,CATTIT
        ADD     HL,DE
        LD      A,(SCRA)
        INC     A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      D,A
        LD      E,3
        LD      A,(SCRB)
        OR      A
        LD      C,CSBND
        JR      Z,TPR3
        LD      C,CSNORM
TPR3:   LD      B,16
        JP      DRWSTR
