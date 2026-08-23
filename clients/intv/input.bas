' input.bas -- edge-detected controller input + shared constants, extracted
' from netcat/intv/kbd.bas (itself ported from fujinet-config/intv), minus
' the on-screen grid keyboard this client doesn't need.

    ' Color-stack mode foreground colors.
    CONST CS_BLACK      = 0
    CONST CS_BLUE       = 1
    CONST CS_RED        = 2
    CONST CS_TAN        = 3
    CONST CS_DARKGREEN  = 4
    CONST CS_GREEN      = 5
    CONST CS_YELLOW     = 6
    CONST CS_WHITE      = 7

    CONST SCREEN_COLS = 20
    DEF FN screenpos(aColumn, aRow) = (((aRow)*SCREEN_COLS)+(aColumn))

    ' Disc directions (as reported by in_poll).
    CONST DISC_UP     = $0004
    CONST DISC_RIGHT  = $0002
    CONST DISC_DOWN   = $0001
    CONST DISC_LEFT   = $0008

    ' Keypad, as decoded by CONT.KEY.
    CONST KEYPAD_0      = 0
    CONST KEYPAD_CLEAR  = 10
    CONST KEYPAD_ENTER  = 11
    CONST KEYPAD_NONE   = 12

    CONST IN_REPEAT_DELAY = 18   ' frames held before auto-repeat (~0.3s)
    CONST IN_REPEAT_RATE  = 6    ' frames between repeats (~0.1s)

    DIM in_disc, in_pdisc, in_rdelay
    DIM in_braw, in_btn, in_pbtn
    DIM in_key, in_pkey

' ---------------------------------------------------------------------------
' in_poll: call once per frame (after WAIT). Sets, per call:
'   in_disc - DISC_UP/DOWN/LEFT/RIGHT on a fresh press or an auto-repeat
'             tick while held; 0 otherwise.
'   in_btn  - 1 on a fresh action-button press (any of B0/B1/B2).
'   in_key  - the decoded keypad value on a fresh press, else KEYPAD_NONE.
' Uses the unqualified CONT.* pseudo-variables (both controllers OR'd).
' ---------------------------------------------------------------------------
in_poll: PROCEDURE
    in_disc = 0
    IF CONT.UP THEN in_disc = DISC_UP
    IF CONT.DOWN THEN in_disc = DISC_DOWN
    IF CONT.LEFT THEN in_disc = DISC_LEFT
    IF CONT.RIGHT THEN in_disc = DISC_RIGHT

    IF in_disc <> 0 THEN
        IF in_disc <> in_pdisc THEN
            in_rdelay = IN_REPEAT_DELAY
        ELSE
            IF in_rdelay > 0 THEN
                in_rdelay = in_rdelay - 1
                in_disc = 0
            ELSE
                in_rdelay = IN_REPEAT_RATE
            END IF
        END IF
    END IF
    in_pdisc = 0
    IF CONT.UP THEN in_pdisc = DISC_UP
    IF CONT.DOWN THEN in_pdisc = DISC_DOWN
    IF CONT.LEFT THEN in_pdisc = DISC_LEFT
    IF CONT.RIGHT THEN in_pdisc = DISC_RIGHT

    in_braw = 0
    IF CONT.B0 OR CONT.B1 OR CONT.B2 THEN in_braw = 1
    IF in_braw <> 0 AND in_pbtn = 0 THEN
        in_btn = 1
    ELSE
        in_btn = 0
    END IF
    in_pbtn = in_braw

    in_key = KEYPAD_NONE
    IF CONT.KEY <> KEYPAD_NONE AND CONT.KEY <> in_pkey THEN
        in_key = CONT.KEY
    END IF
    in_pkey = CONT.KEY
END
