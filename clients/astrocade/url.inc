; url.inc -- building the devicespec for a request.
;
; The URL is streamed straight into the mailbox's TX byte stream a
; character at a time (the card ports' way): it is never assembled in
; RAM. The cart counts the payload itself, so nothing here tracks a
; length; a whole request is ~90 bytes against the 320-byte TX ceiling.
;
; ps=20x8 mirrors the reader: 20 columns of 5x7 text, 8 body rows (rows
; 2-9 of 11). l=3 is the largest list that provably fits the 1KB reply
; window (4 records could reach 1164 bytes; 3 caps at 891).

; BLDURL: stream the devicespec for the pending request. REQTYP 0 = the
; article list for (LPREQ)/(TOPSEL); 1 = article page (APREQ) of the id
; cached in V_REC slot (LISTSEL) -- the ASCII digits verbatim, because
; ids can exceed 16 bits.
BLDURL: LD      A,(UENDPT)      ; "N:" unless the endpoint carries its own
        CP      'N'
        JR      Z,BURL1
        LD      A,'N'
        CALL    FNTXBYT
        LD      A,':'
        CALL    FNTXBYT
BURL1:  LD      HL,UENDPT
        CALL    FNTXSTR
        LD      HL,UQBASE
        CALL    FNTXSTR
        LD      A,(REQTYP)
        OR      A
        JR      NZ,BURLA
        LD      HL,UQLIST       ; list: &l=3&p=<page>&c=<category>
        CALL    FNTXSTR
        LD      HL,(LPREQ)
        CALL    FMTU16
        CALL    FNTXSTR
        LD      HL,UQCAT
        CALL    FNTXSTR
        LD      A,(TOPSEL)
        CALL    MUL14
        LD      DE,CATNAMS
        ADD     HL,DE
        JP      FNTXSTR
BURLA:  LD      HL,UQPAGE       ; article: &p=<page>&a=<id>
        CALL    FNTXSTR
        LD      HL,(APREQ)
        CALL    FMTU16
        CALL    FNTXSTR
        LD      HL,UQART
        CALL    FNTXSTR
        LD      A,(LISTSEL)
        CALL    RECPTR
        JP      FNTXSTR

UQBASE: DB      "?t=lf&ps=20x8",0
UQLIST: DB      "&l=3&p=",0
UQCAT:  DB      "&c=",0
UQPAGE: DB      "&p=",0
UQART:  DB      "&a=",0
