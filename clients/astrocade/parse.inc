; parse.inc -- walking the reply where it lies.
;
; Every cursor is a flat 0-1023 reply-window offset in HL, read through
; state.inc's RXGETB and bounded by RXLEN. RXCUR returns 0 at and past
; the end of the reply, so "NUL or LF" is the field terminator everywhere
; -- the same shape as the Intellivision client's sanitized buffer,
; without the sanitize pass (the reply is cart-side and read-only).

; RXCUR: A = the reply byte at offset HL, or 0 at/past the reply's end.
; Preserves BC, DE, HL.
RXCUR:  PUSH    DE
        PUSH    HL
        EX      DE,HL           ; DE = offset
        LD      HL,(RXLEN)
        OR      A
        SBC     HL,DE
        POP     HL
        POP     DE
        JR      Z,RXC0
        JR      C,RXC0
        JP      RXGETB
RXC0:   XOR     A
        RET

; PU16: DE = the unsigned decimal number at offset HL, stopping at the
; first non-digit (zero if none). Clobbers A.
PU16:   LD      DE,0
PU1:    CALL    RXCUR
        CP      '0'
        RET     C
        CP      '9'+1
        RET     NC
        SUB     '0'
        INC     HL
        PUSH    HL
        LD      H,D
        LD      L,E
        ADD     HL,HL           ; 2x
        LD      D,H
        LD      E,L
        ADD     HL,HL           ; 4x
        ADD     HL,HL           ; 8x
        ADD     HL,DE           ; 10x
        LD      E,A
        LD      D,0
        ADD     HL,DE
        EX      DE,HL           ; DE = accumulator
        POP     HL
        JR      PU1

; SKIPLF: advance HL past the next LF (or to the reply's end).
; Clobbers A.
SKIPLF: CALL    RXCUR
        OR      A
        RET     Z
        CP      0AH
        JR      Z,SKLF1
        INC     HL
        JR      SKIPLF
SKLF1:  INC     HL
        RET

; SKPBAR: advance HL past the next '|'. Carry set if the line or reply
; ended first (malformed record). Clobbers A.
SKPBAR: CALL    RXCUR
        OR      A
        JR      Z,SKBBAD
        CP      0AH
        JR      Z,SKBBAD
        CP      '|'
        JR      Z,SKB1
        INC     HL
        JR      SKPBAR
SKB1:   INC     HL
        OR      A
        RET
SKBBAD: SCF
        RET

; CPYID: copy the digits at offset HL into (DE) up to the '|', at most
; 15 characters (the rest skipped), NUL-terminated; HL is left past the
; '|'. Carry set if the line or reply ended before one (malformed).
; Clobbers A,B,C.
CPYID:  LD      B,16            ; 15 characters + the terminator's slot
CPI1:   CALL    RXCUR
        OR      A
        JR      Z,CPIBAD
        CP      0AH
        JR      Z,CPIBAD
        CP      '|'
        JR      Z,CPIOK
        INC     HL
        LD      C,A
        LD      A,B
        CP      2
        JR      C,CPI1          ; full: keep skipping to the '|'
        LD      A,C
        LD      (DE),A
        INC     DE
        DEC     B
        JR      CPI1
CPIOK:  INC     HL
        XOR     A
        LD      (DE),A
        RET                     ; XOR A cleared carry
CPIBAD: XOR     A
        LD      (DE),A
        SCF
        RET

; CHKERR: Z set when the reply begins "ERROR:" -- the server's error
; shape. Clobbers A,B,DE,HL.
CHKERR: LD      HL,0
        LD      DE,SERRLIT
        LD      B,6
CKE1:   CALL    RXCUR
        EX      DE,HL
        CP      (HL)
        EX      DE,HL
        RET     NZ
        INC     HL
        INC     DE
        DJNZ    CKE1
        XOR     A               ; all six matched
        RET
