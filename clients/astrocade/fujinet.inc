; fujinet.inc -- N: device commands and reply offsets.
;
; Hand-synchronised with the other Astrocade ports (netcat, the card
; games); the canonical list is fujinet-firmware/include/fujiCommandID.h.
; The mailbox transport itself is fujilib.inc.

; ---- Network device (N:) ----------------------------------------------
NETDEV  EQU     71H             ; N1:
NCOPEN  EQU     4FH             ; 'O'
NCCLOSE EQU     43H             ; 'C'
NCREAD  EQU     52H             ; 'R'
NCSTAT  EQU     53H             ; 'S'

NMHTTPG EQU     0CH             ; open mode 12: the HTTP adapter's GET
NTRNONE EQU     0               ; translation: none (the server's t=lf
                                ; already delivers bare-LF line endings)

; NET_STATUS reply: 4 bytes in slice 0.
NSAVLO  EQU     0               ; bytes waiting, low
NSAVHI  EQU     1               ; bytes waiting, high
NSCONN  EQU     2               ; connected flag
NSDEVST EQU     3               ; nDevStatus_t
NDSOK   EQU     1               ; NDEV_STATUS::SUCCESS
NDSEOF  EQU     136             ; NDEV_STATUS::END_OF_FILE -- the server
                                ; closes after sending, so a fully landed
                                ; response can already read as EOF

; ---- Sizes -------------------------------------------------------------
; The cart publishes replies through four 256-byte slices; state.inc's
; RXGETB gives a flat 0-1023 offset space over them. Both news responses
; fit (list: 12 + 3x288 = 876 worst case; article: 714), so one READ
; captures everything and the screens re-read it in place.
RXMAX   EQU     1024            ; FUJIMAIL_RX_MAX: the READ clamp
