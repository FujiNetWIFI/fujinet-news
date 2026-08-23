' st_reader.bas -- article reader screen.
'
' Tan "paper" background with a blue title band (MODE 0,3,1,3,1 -- the
' band is the highlight color, opened at card 0 and closed at card 40, so
' it covers rows 0-1). Body text is drawn black-on-tan exactly as the
' server wrapped it (ps=20x8 = the 8 body rows). Menu row: page indicator
' in blue, key tokens in red, command words in black.
'
' Layout (rows):
'   0-1    title, word-wrapped client-side to 2 rows, white on blue
'   2      blank separator
'   3-10   8 body rows, verbatim from the server
'   11     cur/max ^V:PG 0:INFO
'
' Keypad 0 overlays the article's date and source (rows 2-10); any input
' redraws the body from SC_RESP without refetching.

    DIM rd_i
    DIM #art_pcur, #art_pmax
    DIM #art_date, #art_srcl, #art_body

reader_enter:
    GOSUB art_fetch
    IF fetch_err <> 0 THEN GOTO reader_fail
    GOSUB art_parse

reader_show:
    MODE 0,CS_TAN,CS_BLUE,CS_TAN,CS_BLUE
    WAIT
    CLS
    GOSUB reader_draw

reader_loop:
    WAIT
    GOSUB in_poll
    IF in_disc = DISC_UP THEN GOTO reader_prev
    IF in_disc = DISC_LEFT THEN GOTO reader_prev
    IF in_disc = DISC_DOWN THEN GOTO reader_next
    IF in_disc = DISC_RIGHT THEN GOTO reader_next
    IF in_key = KEYPAD_ENTER THEN GOTO reader_next
    IF in_btn = 1 THEN GOTO reader_next
    IF in_key = KEYPAD_0 THEN GOTO reader_info
    IF in_key = KEYPAD_CLEAR THEN GOTO list_show
    GOTO reader_loop

reader_prev:
    IF #art_pcur > 1 THEN #art_page = #art_pcur - 1 : GOTO reader_enter
    GOTO reader_loop

reader_next:
    IF #art_pcur < #art_pmax THEN #art_page = #art_pcur + 1 : GOTO reader_enter
    GOTO reader_loop

reader_fail:
    ' The failed fetch clobbered SC_RESP (where the body lives), so there
    ' is nothing to redraw here -- fall back to the article list, whose
    ' records in SC_REC are untouched by article fetches.
    GOSUB err_show
    GOTO list_show

' --- Info overlay -----------------------------------------------------------

reader_info:
    FOR rd_i = 2 TO 10
        s_row = rd_i
        GOSUB scr_row_clear
    NEXT rd_i
    ' Row 2's column 0 held the band-closing advance bit -- re-apply it or
    ' the blue band floods the whole overlay.
    GOSUB hl_set
    PRINT AT screenpos(0,3) COLOR CS_RED,"DATE:"
    s_row = 4
    s_col = 0
    s_max = 20
    s_col_color = CS_BLACK
    #s_src = #art_date
    GOSUB scr_puts
    PRINT AT screenpos(0,6) COLOR CS_RED,"SOURCE:"
    s_row = 7
    s_col = 0
    s_max = 20
    s_col_color = CS_BLACK
    #s_src = #art_srcl
    GOSUB scr_puts
    PRINT AT screenpos(6,10) COLOR CS_BLACK,"ANY KEY"
ri_loop:
    WAIT
    GOSUB in_poll
    IF in_btn = 1 THEN GOTO reader_show
    IF in_key <> KEYPAD_NONE THEN GOTO reader_show
    IF in_disc <> 0 THEN GOTO reader_show
    GOTO ri_loop

' --- Fetch / parse ----------------------------------------------------------

art_fetch: PROCEDURE
    GOSUB draw_fetching
    #fn_txlen = 0
    GOSUB url_base
    #fn_src = VARPTR lit_ampp(0) : ls_max = 4 : GOSUB fn_strlen : GOSUB fn_putstr
    #fm_val = #art_page
    GOSUB url_putnum16
    #fn_src = VARPTR lit_ampa(0) : ls_max = 4 : GOSUB fn_strlen : GOSUB fn_putstr
    ' Article id: the ASCII digits captured at list-parse time, verbatim.
    #fn_src = SC_REC + list_sel * 100
    ls_max = 16
    GOSUB fn_strlen
    GOSUB fn_putstr
    GOSUB news_fetch
END

' Response: title / pubdate / source / "page/totalpages" / body rows.
' (All LFs are already NULs from the sanitize pass.)
art_parse: PROCEDURE
    #p_i = SC_RESP
    #w_src = #p_i
    #w_dst = SC_TREC
    w_rows = 2
    GOSUB wrap_text
    #p_i = #w_src + 1
    #art_date = #p_i
    GOSUB skip_field
    #art_srcl = #p_i
    GOSUB skip_field
    GOSUB parse_u16
    #art_pcur = #p_val
    IF (PEEK(#p_i) AND 255) = 47 THEN #p_i = #p_i + 1
    GOSUB parse_u16
    #art_pmax = #p_val
    GOSUB skip_field
    #art_body = #p_i
    IF #art_pcur = 0 THEN #art_pcur = 1
    IF #art_pmax = 0 THEN #art_pmax = 1
END

' --- Drawing ----------------------------------------------------------------

reader_draw: PROCEDURE
    FOR rd_i = 0 TO 1
        s_row = rd_i
        s_col = 0
        s_max = 20
        s_col_color = CS_WHITE
        #s_src = SC_TREC + rd_i * 21
        GOSUB scr_puts
    NEXT rd_i
    s_row = 2
    GOSUB scr_row_clear
    #p_i = #art_body
    FOR rd_i = 0 TO 7
        s_row = 3 + rd_i
        IF #p_i < (SC_RESP + #resp_len) THEN
            s_col = 0
            s_max = 20
            s_col_color = CS_BLACK
            #s_src = #p_i
            GOSUB scr_puts
            GOSUB skip_field
        ELSE
            GOSUB scr_row_clear
        END IF
    NEXT rd_i
    GOSUB reader_menu
    ' Title band advance bits LAST (everything above writes whole words).
    #hl_a = 0
    #hl_b = 40
    GOSUB hl_set
END

reader_menu: PROCEDURE
    s_row = 11
    GOSUB scr_row_clear
    #pg_a = #art_pcur
    #pg_b = #art_pmax
    GOSUB pg_build
    s_row = 11
    s_col = 0
    s_max = 7
    s_col_color = CS_BLUE
    #s_src = SC_PAGE
    GOSUB scr_puts
    PRINT AT screenpos(8,11) COLOR CS_RED,"^V"
    PRINT COLOR CS_BLACK,":PG "
    PRINT COLOR CS_RED,"0"
    PRINT COLOR CS_BLACK,":INFO"
END
