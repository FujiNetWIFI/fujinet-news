' screen.bas -- low-level text drawing helpers, adapted from
' fujinet-lobby/intv/screen.bas.
'
' Character encoding follows the standard IntyBASIC card formula used
' throughout the FujiNet Intellivision tree:
'     card = ascii - 32   (clamped to space if out of GROM range)
'     screen word = card*8 + color
' Nothing here ever buffers a whole string in an IntyBASIC variable -- text
' is always drawn straight from a source address (ROM DATA or scratch RAM)
' via PEEK, one character at a time, per the RAM budget rule.
'
' One deviation from the lobby original: the printable clamp stops at 95,
' not 126 -- in color-stack mode only GROM cards 0-63 (ASCII 32-95) render
' as text, so anything above (lowercase survivors, `{|}~`) draws as space.

    DIM s_row, s_col, s_i, s_c, s_len, s_max, s_col_color
    DIM #s_src
    DIM #hl_a, #hl_b

' ---------------------------------------------------------------------------
' scr_row_clear: blank row s_row (all 20 columns). Writes whole words, so it
' also wipes any color-stack advance bit sitting on this row -- re-apply
' highlight bits after clearing (see hl_set below).
' ---------------------------------------------------------------------------
scr_row_clear: PROCEDURE
    FOR s_i = 0 TO SCREEN_COLS - 1
        #BACKTAB(s_row * SCREEN_COLS + s_i) = 0
    NEXT s_i
END

' ---------------------------------------------------------------------------
' scr_puts: draw bytes from #s_src onto row s_row starting at column s_col,
' in color s_col_color, stopping at the first NUL or after s_max characters,
' then space-padding the remainder of the s_max-wide field. Sets s_len to
' the number of real (non-pad) characters drawn. Caller ensures
' s_col + s_max <= SCREEN_COLS. Writes whole words (wipes advance bits).
' ---------------------------------------------------------------------------
scr_puts: PROCEDURE
    s_len = 0
    WHILE (s_len < s_max) AND ((PEEK(#s_src + s_len) AND 255) <> 0)
        s_len = s_len + 1
    WEND
    FOR s_i = 0 TO s_max - 1
        IF s_i < s_len THEN
            s_c = PEEK(#s_src + s_i) AND 255
        ELSE
            s_c = 32
        END IF
        IF s_c < 32 THEN s_c = 32
        IF s_c > 95 THEN s_c = 32
        #BACKTAB(s_row * SCREEN_COLS + s_col + s_i) = (s_c - 32) * 8 + s_col_color
    NEXT s_i
END

' ---------------------------------------------------------------------------
' Row-highlight bar via the color-stack advance bit. The screens run
' MODE 0,bg,bar,bg,bar: setting bit 13 ($2000) on the first card of a
' region advances the stack to the bar color, and setting it on the first
' card AFTER the region advances back to bg. #hl_a/#hl_b hold the two card
' indices of the currently-set pair, so hl_clear can undo exactly what
' hl_set did.
'
' Two rules, both load-bearing:
'   1. Apply bits LAST in every draw path -- scr_puts/scr_row_clear/PRINT
'      write whole words and silently wipe them.
'   2. Keep exactly one open/close pair live per screen, or the stack
'      doesn't return to bg for the rest of the display.
' ---------------------------------------------------------------------------
hl_set: PROCEDURE
    #BACKTAB(#hl_a) = #BACKTAB(#hl_a) OR $2000
    #BACKTAB(#hl_b) = #BACKTAB(#hl_b) OR $2000
END

hl_clear: PROCEDURE
    #BACKTAB(#hl_a) = #BACKTAB(#hl_a) AND $DFFF
    #BACKTAB(#hl_b) = #BACKTAB(#hl_b) AND $DFFF
END
