; net.inc -- the N: device round trip.
;
; The card ports' proven one-shot HTTP GET sequence (5cardstud's
; APICALL), over fujilib's mailbox. The rules it carries forward:
;
;  * The CLOSE belongs at the START of the next request, never after the
;    READ: every transaction repaints the whole reply window, a CLOSE
;    reply is empty, and every screen renders straight out of the window
;    between fetches.
;  * FNRXLL/FNRXLH belong to the most recent transaction, so the reply
;    length is captured immediately after the READ.
;  * The HTTP GET is deferred until the first STATUS, and the ESP32
;    reports STATUS as soon as SOME of the response has arrived; poll
;    until two consecutive readings agree (bounded) rather than guessing
;    a delay.
;  * The 4th STATUS reply byte is nDevStatus_t and checking it is not
;    optional: an HTTP error page still has a readable body. The server
;    closes after sending, so END_OF_FILE is as good as SUCCESS here.
;
; All routines return carry SET on failure, clear on success.

; NCMD: begin a network-device transaction. E = command, L = nparam.
NCMD:   LD      A,NETDEV
        JP      FNBEGIN

; NCLOSE: close the previous request's connection. A first call has
; nothing to close and earns a NAK; both results are ignored.
NCLOSE: LD      E,NCCLOSE
        LD      L,0
        CALL    NCMD
        LD      B,3             ; ~6 s
        JP      FNCOMMIT

; NOPEN: OPEN the devicespec BLDURL streams, as HTTP GET, no translation.
NOPEN:  LD      E,NCOPEN
        LD      L,2
        CALL    NCMD
        LD      A,NMHTTPG
        CALL    FNPARB
        LD      A,NTRNONE
        CALL    FNPARB
        CALL    BLDURL
        LD      B,8             ; HTTPS handshakes take a while
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NOBAD
        LD      A,(FNREPLY)
        CP      FCACK
        RET     Z               ; equal: carry already clear
NOBAD:  SCF
        RET

; NSTATUS: one STATUS poll. On success AVAIL = bytes waiting.
NSTATUS: LD     E,NCSTAT
        LD      L,2
        CALL    NCMD
        XOR     A
        CALL    FNPARB
        XOR     A
        CALL    FNPARB
        LD      B,4
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NSBAD
        LD      A,(FNREPLY)
        CP      FCACK
        JR      NZ,NSBAD
        LD      A,(FNRDATA+NSDEVST)
        CP      NDSOK
        JR      Z,NSOK1
        CP      NDSEOF
        JR      NZ,NSBAD
NSOK1:  LD      A,(FNRDATA+NSAVLO)
        LD      (AVAIL),A
        LD      A,(FNRDATA+NSAVHI)
        LD      (AVAIL+1),A
        OR      A               ; clear carry
        RET
NSBAD:  SCF
        RET

; NREAD: READ min(AVAIL, RXMAX) bytes into the reply window, then
; capture the reply length and re-home the slice cursor.
NREAD:  LD      HL,(AVAIL)
        LD      DE,RXMAX
        OR      A
        SBC     HL,DE
        JR      C,NRD1          ; avail < clamp: take avail
        LD      H,D
        LD      L,E
        JR      NRD2
NRD1:   ADD     HL,DE
NRD2:   PUSH    HL
        LD      E,NCREAD
        LD      L,1
        CALL    NCMD
        LD      A,2             ; one 2-byte parameter: the length, LE
        CALL    FNTXBYT
        POP     HL
        PUSH    HL
        LD      A,L
        CALL    FNTXBYT
        POP     HL
        LD      A,H
        CALL    FNTXBYT
        LD      B,8
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NRBAD
        LD      A,(FNREPLY)
        CP      FCACK
        JR      NZ,NRBAD
        LD      A,(FNRXLL)      ; capture NOW: the next transaction's
        LD      (RXLEN),A       ; reply length would replace it
        LD      A,(FNRXLH)
        LD      (RXLEN+1),A
        XOR     A
        LD      (CURSLC),A      ; the cart republished slice 0
        RET                     ; carry clear (XOR A)
NRBAD:  SCF
        RET

; ----------------------------------------------------------------------
; NEWSGET: one full round trip for the request BLDURL streams (REQTYP
; already set). On success (carry clear) the reply is in the window with
; the slice cursor homed, and stays valid until the next NEWSGET -- no
; mailbox transaction may run in between, or the window is clobbered.
NEWSGET: CALL   NCLOSE          ; the PREVIOUS request's connection
        CALL    NOPEN
        RET     C
        XOR     A
        LD      (PRVAVL),A
        LD      (PRVAVL+1),A
        LD      B,20            ; settle-loop bound
NGSET:  PUSH    BC
        CALL    NSTATUS
        POP     BC
        JR      C,NGFL
        LD      HL,(AVAIL)
        LD      A,H
        OR      L
        JR      Z,NGAGN         ; nothing has arrived at all yet
        LD      DE,(PRVAVL)
        OR      A
        SBC     HL,DE
        JR      Z,NGRD          ; two readings agree: it has landed
NGAGN:  LD      HL,(AVAIL)
        LD      (PRVAVL),HL
        LD      DE,4000         ; ~3 frames between polls
NGDLY:  DEC     DE
        LD      A,D
        OR      E
        JR      NZ,NGDLY
        DJNZ    NGSET
NGFL:   CALL    NCLOSE          ; failed before the READ: nothing to keep
        SCF
        RET
NGRD:   CALL    NREAD
        JR      C,NGFL
        RET

; FETCH: NEWSGET plus classification into FERR: 0 good, 1 transport
; failure / empty / truncated (a reply that exactly fills the window
; cannot be complete -- the real maximum is 891), 2 the server's own
; ERROR: line (left in the window for ERRSHOW).
FETCH:  CALL    NEWSGET
        JR      C,FTBAD
        LD      HL,(RXLEN)
        LD      A,H
        OR      L
        JR      Z,FTBAD
        LD      DE,RXMAX
        OR      A
        SBC     HL,DE
        JR      Z,FTBAD
        CALL    CHKERR
        JR      Z,FTERR
        XOR     A
        LD      (FERR),A
        RET
FTBAD:  LD      A,1
        LD      (FERR),A
        RET
FTERR:  LD      A,2
        LD      (FERR),A
        RET
