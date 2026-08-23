' st_list.bas -- article-list screen.
'
' Black background, blue selection bar (MODE 0,0,1,0,1). Three articles
' per page (server l=3), each headline word-wrapped client-side across 3
' rows of 20 (wrap.bas). Header row: category title left (yellow), page
' indicator right (white). Rows 10-11: menus with yellow key tokens.
'
' Goto-page: any keypad digit 1-9 starts numeric entry echoed on row 11;
' Enter (or an action button) jumps, Clear cancels.
'
' Layout (rows):
'   0      header: CATEGORY          cur/max
'   1-9    3 records x 3 wrapped headline rows
'   10     ^V:PICK <>:PG #:GOTO
'   11     ENT:READ  CLR:TOPICS
' Bar: advance bits at cards (1+sel*3)*20 and (4+sel*3)*20. For sel=2 the
' closing bit lands on row 10 column 0 -- still on-screen, by design.

    DIM arts_on_page, rec_ok, rc_n, p_r, ls_i, ls_j, goto_len
    DIM #list_pcur, #list_pmax, #goto_val, #rc_dst

list_enter:
    GOSUB list_fetch
    IF fetch_err <> 0 THEN GOTO list_fetch_fail
    GOSUB list_parse
    IF arts_on_page = 0 THEN fetch_err = 1 : GOTO list_fetch_fail
    list_valid = 1
    IF list_sel >= arts_on_page THEN list_sel = arts_on_page - 1

list_show:
    MODE 0,CS_BLACK,CS_BLUE,CS_BLACK,CS_BLUE
    WAIT
    CLS
    GOSUB list_draw

list_loop:
    WAIT
    GOSUB in_poll
    IF in_disc = DISC_UP THEN GOTO list_up
    IF in_disc = DISC_DOWN THEN GOTO list_down
    IF in_disc = DISC_LEFT THEN GOTO list_prev
    IF in_disc = DISC_RIGHT THEN GOTO list_next
    IF in_btn = 1 THEN GOTO list_read
    IF in_key = KEYPAD_ENTER THEN GOTO list_read
    IF in_key = KEYPAD_CLEAR THEN GOTO topics_enter
    IF (in_key >= 1) AND (in_key <= 9) THEN GOTO list_goto_start
    GOTO list_loop

list_up:
    IF list_sel > 0 THEN GOSUB hl_clear : list_sel = list_sel - 1 : GOSUB list_bar_set : GOTO list_loop
    ' Up past the top: previous page, cursor at the bottom (coco behavior).
    IF #list_pcur > 1 THEN list_sel = 2 : #list_page = #list_pcur - 1 : GOTO list_enter
    GOTO list_loop

list_down:
    IF list_sel < (arts_on_page - 1) THEN GOSUB hl_clear : list_sel = list_sel + 1 : GOSUB list_bar_set : GOTO list_loop
    IF #list_pcur < #list_pmax THEN list_sel = 0 : #list_page = #list_pcur + 1 : GOTO list_enter
    GOTO list_loop

list_prev:
    IF #list_pcur > 1 THEN #list_page = #list_pcur - 1 : GOTO list_enter
    GOTO list_loop

list_next:
    IF #list_pcur < #list_pmax THEN #list_page = #list_pcur + 1 : GOTO list_enter
    GOTO list_loop

list_read:
    #art_page = 1
    GOTO reader_enter

' --- Goto-page numeric entry -----------------------------------------------

list_goto_start:
    #goto_val = in_key
    goto_len = 1
    GOSUB list_goto_draw

list_goto_loop:
    WAIT
    GOSUB in_poll
    IF in_key <= 9 THEN GOSUB list_goto_digit
    IF in_key = KEYPAD_ENTER THEN GOTO list_goto_go
    IF in_btn = 1 THEN GOTO list_goto_go
    IF in_key = KEYPAD_CLEAR THEN GOSUB list_menu : GOSUB hl_set : GOTO list_loop
    GOTO list_goto_loop

list_goto_go:
    IF #goto_val < 1 THEN #goto_val = 1
    IF #goto_val > #list_pmax THEN #goto_val = #list_pmax
    IF #goto_val = #list_pcur THEN GOSUB list_menu : GOSUB hl_set : GOTO list_loop
    #list_page = #goto_val
    GOTO list_enter

list_goto_digit: PROCEDURE
    IF goto_len < 4 THEN
        #goto_val = #goto_val * 10 + in_key
        goto_len = goto_len + 1
        GOSUB list_goto_draw
    END IF
END

list_goto_draw: PROCEDURE
    s_row = 11
    GOSUB scr_row_clear
    PRINT AT screenpos(0,11) COLOR CS_YELLOW,"GOTO PAGE: "
    PRINT COLOR CS_WHITE,<>#goto_val
END

' --- Fetch / parse ----------------------------------------------------------

list_fetch: PROCEDURE
    GOSUB draw_fetching
    #fn_txlen = 0
    GOSUB url_base
    #fn_src = VARPTR lit_listp(0) : ls_max = 8 : GOSUB fn_strlen : GOSUB fn_putstr
    #fm_val = #list_page
    GOSUB url_putnum16
    #fn_src = VARPTR lit_ampc(0) : ls_max = 4 : GOSUB fn_strlen : GOSUB fn_putstr
    #fn_src = VARPTR lit_cats(0) + topic_sel * 14
    ls_max = 14
    GOSUB fn_strlen
    GOSUB fn_putstr
    GOSUB news_fetch
END

' Response: line 1 "page/totalpages", then up to 3 lines of
' "id|YYYY-MM-DD HH:MM:SS|headline" (LFs already turned into NULs by the
' sanitize pass, so each line is a NUL-terminated field).
list_parse: PROCEDURE
    #p_i = SC_RESP
    GOSUB parse_u16
    #list_pcur = #p_val
    IF (PEEK(#p_i) AND 255) = 47 THEN #p_i = #p_i + 1
    GOSUB parse_u16
    #list_pmax = #p_val
    GOSUB skip_field
    IF #list_pcur = 0 THEN #list_pcur = 1
    IF #list_pmax = 0 THEN #list_pmax = 1
    arts_on_page = 0
    FOR p_r = 0 TO 2
        GOSUB list_parse_rec
        IF rec_ok = 0 THEN EXIT FOR
        arts_on_page = arts_on_page + 1
    NEXT p_r
END

' Parse one "id|timestamp|headline" record into SC_REC slot p_r:
'   +0  id, ASCII digits, NUL-terminated (kept as text -- ids can exceed
'       16 bits, and the article URL wants the digits verbatim anyway)
'   +16 timestamp, NUL-terminated (info display / future use)
'   +36 headline word-wrapped to 3 rows of stride 21
' rec_ok = 0 on a missing/malformed record (stops the page).
list_parse_rec: PROCEDURE
    rec_ok = 0
    IF #p_i >= (SC_RESP + #resp_len) THEN RETURN
    #rc_dst = SC_REC + p_r * 100
    rc_n = 0
lpr_id:
    #fm_val = PEEK(#p_i) AND 255
    IF #fm_val = 0 THEN RETURN
    IF #fm_val = 124 THEN GOTO lpr_id_done
    IF rc_n < 15 THEN POKE #rc_dst + rc_n, #fm_val : rc_n = rc_n + 1
    #p_i = #p_i + 1
    GOTO lpr_id
lpr_id_done:
    POKE #rc_dst + rc_n, 0
    #p_i = #p_i + 1
    rc_n = 0
lpr_ts:
    #fm_val = PEEK(#p_i) AND 255
    IF #fm_val = 0 THEN RETURN
    IF #fm_val = 124 THEN GOTO lpr_ts_done
    IF rc_n < 19 THEN POKE #rc_dst + 16 + rc_n, #fm_val : rc_n = rc_n + 1
    #p_i = #p_i + 1
    GOTO lpr_ts
lpr_ts_done:
    POKE #rc_dst + 16 + rc_n, 0
    #p_i = #p_i + 1
    #w_src = #p_i
    #w_dst = #rc_dst + 36
    w_rows = 3
    GOSUB wrap_text
    #p_i = #w_src + 1
    rec_ok = 1
END

' --- Drawing ----------------------------------------------------------------

list_draw: PROCEDURE
    s_row = 0
    s_col = 0
    s_max = 13
    s_col_color = CS_YELLOW
    #s_src = VARPTR lit_titles(0) + topic_sel * 14
    GOSUB scr_puts
    #pg_a = #list_pcur
    #pg_b = #list_pmax
    GOSUB pg_build
    #fn_src = SC_PAGE : ls_max = 12 : GOSUB fn_strlen
    s_row = 0
    s_col = 20 - fn_len
    s_max = fn_len
    s_col_color = CS_WHITE
    #s_src = SC_PAGE
    GOSUB scr_puts

    FOR ls_i = 0 TO 2
        FOR ls_j = 0 TO 2
            s_row = 1 + ls_i * 3 + ls_j
            IF ls_i < arts_on_page THEN
                s_col = 0
                s_max = 20
                s_col_color = CS_WHITE
                #s_src = SC_REC + ls_i * 100 + 36 + ls_j * 21
                GOSUB scr_puts
            ELSE
                GOSUB scr_row_clear
            END IF
        NEXT ls_j
    NEXT ls_i

    GOSUB list_menu
    GOSUB list_bar_set
END

list_menu: PROCEDURE
    s_row = 10
    GOSUB scr_row_clear
    s_row = 11
    GOSUB scr_row_clear
    PRINT AT screenpos(0,10) COLOR CS_YELLOW,"^V"
    PRINT COLOR CS_WHITE,":PICK "
    PRINT COLOR CS_YELLOW,"<>"
    PRINT COLOR CS_WHITE,":PG "
    PRINT COLOR CS_YELLOW,"#"
    PRINT COLOR CS_WHITE,":GOTO"
    PRINT AT screenpos(0,11) COLOR CS_YELLOW,"ENT"
    PRINT COLOR CS_WHITE,":READ  "
    PRINT COLOR CS_YELLOW,"CLR"
    PRINT COLOR CS_WHITE,":TOPICS"
END

list_bar_set: PROCEDURE
    #hl_a = (1 + list_sel * 3) * 20
    #hl_b = (4 + list_sel * 3) * 20
    GOSUB hl_set
END

list_fetch_fail:
    GOSUB err_show
    ' A failed page flip leaves the old records in SC_REC untouched --
    ' fall back to the page we were on. With nothing valid yet (straight
    ' from the topics screen), go back there instead.
    IF list_valid = 1 THEN #list_page = #list_pcur : GOTO list_show
    GOTO topics_enter
