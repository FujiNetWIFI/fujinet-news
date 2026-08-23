' news.bas -- FujiNet News for the Intellivision, entry point.
'
' A three-screen reader for the 8bitnews service at fujinet.online, over
' the FujiNet mailbox transport (fujinet.bas, the shared library from
' netcat/intv -- see its header for the $9C00 mailbox contract):
'
'   topics (st_topics.bas)  blue bg   pick a category, keypad 1-9 or disc
'   list   (st_list.bas)    black bg  3 word-wrapped headlines per page
'   reader (st_reader.bas)  tan bg    server-wrapped body, blue title band
'
' The server does the article-body word wrap (ps=20x8 in the URL) and the
' pagination; the client word-wraps only headlines and the title (wrap.bas).
' Each screen sets its own background via MODE 0 and gets its selection
' bar / title band from the color-stack advance bit (screen.bas).
'
' Scratch cart RAM map ($9000-$97FF convention from the other clients;
' $9100-$917F belongs to fujinet.bas):
'   SC_RESP $9200-$97FF  raw server response, NUL-terminated, LFs turned
'                        into NULs so every line is a drawable field
'   SC_REC  $9800-$992B  3 list records, stride 100: id ASCII at +0,
'                        timestamp at +16, wrapped headline 3x21 at +36
'   SC_TREC $9930-$995F  reader title wrapped to 2x21
'   SC_NUM  $9960-$9967  fmt_u16 digit scratch
'   SC_PAGE $9970-$997F  "cur/max" page-indicator string

    CONST SC_RESP  = $9200
    CONST RESP_CAP = 1535
    CONST SC_REC   = $9800
    CONST SC_TREC  = $9930
    CONST SC_NUM   = $9960
    CONST SC_PAGE  = $9970

    DIM fetch_err, fe_i, fe_m, list_valid
    ' Cross-screen state, declared here because earlier includes reference
    ' variables the later screens own (topics seeds the list page, the
    ' list seeds the article page).
    DIM list_sel
    DIM #list_page, #art_page
    DIM #resp_len, #fc_i
    DIM #fm_val, #fm_p
    DIM #sa_src, #sa_dst
    DIM #p_i, #p_val
    DIM #pg_a, #pg_b

' Execution is sequential in IntyBASIC, and INCLUDE pastes files in
' verbatim -- falling into a PROCEDURE or DATA body by straight-line
' execution corrupts the return stack. So every INCLUDE happens here,
' ahead of a GOTO that jumps clear of all of them before real code starts
' (same structure as netcat and fujinet-lobby).
    GOTO nw_start

    INCLUDE "fujinet.bas"
    INCLUDE "input.bas"
    INCLUDE "screen.bas"
    INCLUDE "wrap.bas"
    INCLUDE "st_topics.bas"
    INCLUDE "st_list.bas"

    ' The default $5000-$6FFF segment cannot hold everything, and letting
    ' the compiler auto-continue into $7000 produces a cart that fails
    ' EXEC's boot detection (fujinet-lobby documents the same move). Park
    ' the rest in the $C100-$FFFF range.
    ASM ORG $D000
    INCLUDE "st_reader.bas"

' --- Boot -------------------------------------------------------------------

nw_start:
    MODE 0,CS_BLACK,CS_BLACK,CS_BLACK,CS_BLACK
    WAIT
    CLS
    PRINT AT screenpos(4,3) COLOR CS_YELLOW,"FUJINET NEWS"
    PRINT AT screenpos(0,5) COLOR CS_WHITE,"WAITING FOR FUJINET"
    GOSUB fn_wait_mailbox
    IF fn_ok = 1 THEN GOTO nw_ready
    PRINT AT screenpos(0,7) COLOR CS_RED,"FUJINET NOT FOUND"
    PRINT AT screenpos(0,8) COLOR CS_WHITE,"ANY KEY RETRIES"
nw_retry_wait:
    WAIT
    GOSUB in_poll
    IF in_btn = 1 THEN GOTO nw_start
    IF in_key <> KEYPAD_NONE THEN GOTO nw_start
    IF in_disc <> 0 THEN GOTO nw_start
    GOTO nw_retry_wait

nw_ready:
    topic_sel = 0
    GOTO topics_enter

' --- Shared network layer ---------------------------------------------------

' news_fetch: run the URL already staged at FN_TX/#fn_txlen through
' open -> settle -> read-loop -> close, assembling the whole response in
' SC_RESP (responses exceed the 512-byte mailbox RX window, so this loops
' where fujinet.bas's api_call reads once). Then sanitize in place and
' check for a server "ERROR:" line.
'
' fetch_err: 0 = good response, 1 = transport failure or empty response,
'            2 = server ERROR line (its text is at SC_RESP for display).
' On success SC_RESP holds #resp_len bytes, NUL-terminated, with every LF
' replaced by NUL -- each response line is directly drawable via scr_puts.
news_fetch: PROCEDURE
    fetch_err = 0
    #resp_len = 0
    GOSUB net_open
    IF fn_ok = 0 THEN fetch_err = 1 : RETURN

    ' For HTTP the actual GET fires on this first STATUS, and STATUS
    ' reports bytes-so-far: treat two consecutive equal nonzero readings
    ' as settled (api_call's idiom), but give a real internet fetch up to
    ' ~5s instead of api_call's ~0.33s before declaring it empty.
    #ac_prev = 0
    FOR #fc_i = 0 TO 299
        WAIT
        GOSUB net_status
        IF fn_ok = 0 THEN GOTO nf_finish
        IF (#net_avail > 0) AND (#net_avail = #ac_prev) THEN GOTO nf_read
        #ac_prev = #net_avail
    NEXT #fc_i

nf_read:
    WHILE (fn_ok = 1) AND (#net_avail > 0) AND (#resp_len < RESP_CAP)
        #net_readlen = #net_avail
        IF #net_readlen > 512 THEN #net_readlen = 512
        IF #net_readlen > (RESP_CAP - #resp_len) THEN #net_readlen = RESP_CAP - #resp_len
        GOSUB net_read
        IF fn_ok = 0 THEN GOTO nf_finish
        IF #net_gotlen = 0 THEN GOTO nf_finish
        FOR #fc_i = 0 TO #net_gotlen - 1
            POKE SC_RESP + #resp_len + #fc_i, PEEK(FN_RX + #fc_i) AND 255
        NEXT #fc_i
        #resp_len = #resp_len + #net_gotlen
        ' fn_ok drops to 0 here at EOF (#net_err <> 1 once the connection
        ' drains) -- that ends the loop as end-of-stream, not failure.
        GOSUB net_status
    WEND

nf_finish:
    GOSUB net_close
    POKE SC_RESP + #resp_len, 0
    IF #resp_len = 0 THEN fetch_err = 1 : RETURN

    ' Sanitize in place: fold a-z to A-Z (GROM has no lowercase), LF
    ' becomes NUL (turning each line into a NUL-terminated field), CR and
    ' remaining control bytes become spaces. 96-126 pass through -- the
    ' list parser needs '|' (124) alive; scr_puts blanks strays on screen.
    FOR #fc_i = 0 TO #resp_len - 1
        #fm_val = PEEK(SC_RESP + #fc_i) AND 255
        IF (#fm_val >= 97) AND (#fm_val <= 122) THEN #fm_val = #fm_val - 32
        IF #fm_val = 10 THEN #fm_val = 0
        IF #fm_val = 13 THEN #fm_val = 32
        IF (#fm_val < 32) AND (#fm_val <> 0) THEN #fm_val = 32
        POKE SC_RESP + #fc_i, #fm_val
    NEXT #fc_i

    ' A response starting "ERROR:" is the server's single-line failure
    ' (bad page, empty category, unknown article).
    IF #resp_len >= 6 THEN
        fe_m = 1
        FOR fe_i = 0 TO 5
            #fm_val = PEEK(SC_RESP + fe_i) AND 255
            #p_val = PEEK(VARPTR lit_error(0) + fe_i) AND 255
            IF #fm_val <> #p_val THEN fe_m = 0
        NEXT fe_i
        IF fe_m = 1 THEN fetch_err = 2
    END IF
END

' --- Shared parse helpers ---------------------------------------------------

' parse_u16: read decimal digits at #p_i into #p_val, advancing #p_i past
' them. Stops at the first non-digit. No overflow guard -- page counts
' beyond 65535 wrap, which the server never produces.
parse_u16: PROCEDURE
    #p_val = 0
pu_loop:
    #fm_val = PEEK(#p_i) AND 255
    IF #fm_val < 48 THEN RETURN
    IF #fm_val > 57 THEN RETURN
    #p_val = #p_val * 10 + (#fm_val - 48)
    #p_i = #p_i + 1
    GOTO pu_loop
END

' skip_field: advance #p_i past the current NUL-terminated field and its
' NUL, bounded by the end of the response.
skip_field: PROCEDURE
    WHILE (#p_i < (SC_RESP + #resp_len)) AND ((PEEK(#p_i) AND 255) <> 0)
        #p_i = #p_i + 1
    WEND
    #p_i = #p_i + 1
END

' --- URL / number formatting ------------------------------------------------

' url_base: start a fresh URL in FN_TX with the base devicespec + the
' fixed query prefix (t=lf, ps=20x8). Caller appends &-params after.
url_base: PROCEDURE
    #fn_src = VARPTR lit_base(0)
    ls_max = 60
    GOSUB fn_strlen
    GOSUB fn_putstr
END

' fmt_u16: render #fm_val as decimal ASCII, NUL-terminated, built backward
' in SC_NUM. The first digit's address lands in #fm_p. Destroys #fm_val.
fmt_u16: PROCEDURE
    #fm_p = SC_NUM + 6
    POKE #fm_p, 0
fu_loop:
    #fm_p = #fm_p - 1
    POKE #fm_p, (#fm_val % 10) + 48
    #fm_val = #fm_val / 10
    IF #fm_val > 0 THEN GOTO fu_loop
END

' url_putnum16: append decimal #fm_val to the URL in FN_TX. (fujinet.bas's
' fn_putnum caps at 999; page numbers need the full 16-bit range.)
url_putnum16: PROCEDURE
    GOSUB fmt_u16
    #fn_src = #fm_p
    ls_max = 6
    GOSUB fn_strlen
    GOSUB fn_putstr
END

' sa_append: copy the NUL-terminated string at #sa_src to #sa_dst, leaving
' #sa_dst on the (written) trailing NUL so calls chain.
sa_append: PROCEDURE
    WHILE (PEEK(#sa_src) AND 255) <> 0
        POKE #sa_dst, PEEK(#sa_src) AND 255
        #sa_src = #sa_src + 1
        #sa_dst = #sa_dst + 1
    WEND
    POKE #sa_dst, 0
END

' pg_build: build "#pg_a/#pg_b" NUL-terminated at SC_PAGE.
pg_build: PROCEDURE
    #fm_val = #pg_a
    GOSUB fmt_u16
    #sa_src = #fm_p
    #sa_dst = SC_PAGE
    GOSUB sa_append
    POKE #sa_dst, 47
    #sa_dst = #sa_dst + 1
    #fm_val = #pg_b
    GOSUB fmt_u16
    #sa_src = #fm_p
    GOSUB sa_append
END

' --- Shared UI --------------------------------------------------------------

draw_fetching: PROCEDURE
    CLS
    PRINT AT screenpos(4,5) COLOR CS_WHITE,"FETCHING..."
END

' err_show: display the failure (the server's own ERROR line when there is
' one, else a generic message) and wait for any input. Caller decides
' which screen to fall back to.
err_show: PROCEDURE
    CLS
    IF fetch_err = 2 THEN
        s_row = 4
        s_col = 0
        s_max = 20
        s_col_color = CS_RED
        #s_src = SC_RESP
        GOSUB scr_puts
    ELSE
        PRINT AT screenpos(3,4) COLOR CS_RED,"FETCH FAILED"
    END IF
    PRINT AT screenpos(6,8) COLOR CS_WHITE,"ANY KEY"
es_loop:
    WAIT
    GOSUB in_poll
    IF in_btn = 1 THEN RETURN
    IF in_key <> KEYPAD_NONE THEN RETURN
    IF in_disc <> 0 THEN RETURN
    GOTO es_loop
END

' --- ROM literals -----------------------------------------------------------
' Raw ASCII bytes, NUL-terminated: DATA "strings" store card codes, not
' ASCII, and the wire (URLs) and the parsers need true ASCII. Generated
' from the quoted strings in the comments.

    ' "N:HTTPS://FUJINET.ONLINE/8bitnews/news.php?t=lf&ps=20x8"
lit_base:
    DATA 78,58,72,84,84,80,83,58,47,47,70,85,74,73
    DATA 78,69,84,46,79,78,76,73,78,69,47,56,98,105
    DATA 116,110,101,119,115,47,110,101,119,115,46,112,104,112
    DATA 63,116,61,108,102,38,112,115,61,50,48,120,56,0
    ' "&l=3&p="
lit_listp:
    DATA 38,108,61,51,38,112,61,0
    ' "&c="
lit_ampc:
    DATA 38,99,61,0
    ' "&p="
lit_ampp:
    DATA 38,112,61,0
    ' "&a="
lit_ampa:
    DATA 38,97,61,0
    ' "ERROR:"
lit_error:
    DATA 69,82,82,79,82,58,0
    ' Server category names, stride 14, NUL-terminated (lowercase ASCII as
    ' the server expects in c=). Order matches the coco client's
    ' category_name_to_num and the lit_titles table below.
lit_cats:
    DATA 116,111,112,0,0,0,0,0,0,0,0,0,0,0  ' top
    DATA 119,111,114,108,100,0,0,0,0,0,0,0,0,0  ' world
    DATA 98,117,115,105,110,101,115,115,0,0,0,0,0,0  ' business
    DATA 115,99,105,101,110,99,101,0,0,0,0,0,0,0  ' science
    DATA 116,101,99,104,110,111,108,111,103,121,0,0,0,0  ' technology
    DATA 104,101,97,108,116,104,0,0,0,0,0,0,0,0  ' health
    DATA 101,110,116,101,114,116,97,105,110,109,101,110,116,0  ' entertainment
    DATA 112,111,108,105,116,105,99,115,0,0,0,0,0,0  ' politics
    DATA 115,112,111,114,116,115,0,0,0,0,0,0,0,0  ' sports
    ' Display titles, stride 14, NUL-terminated (uppercase ASCII).
lit_titles:
    DATA 84,79,80,32,83,84,79,82,73,69,83,0,0,0  ' TOP STORIES
    DATA 87,79,82,76,68,32,78,69,87,83,0,0,0,0  ' WORLD NEWS
    DATA 66,85,83,73,78,69,83,83,0,0,0,0,0,0  ' BUSINESS
    DATA 83,67,73,69,78,67,69,0,0,0,0,0,0,0  ' SCIENCE
    DATA 84,69,67,72,78,79,76,79,71,89,0,0,0,0  ' TECHNOLOGY
    DATA 72,69,65,76,84,72,0,0,0,0,0,0,0,0  ' HEALTH
    DATA 69,78,84,69,82,84,65,73,78,77,69,78,84,0  ' ENTERTAINMENT
    DATA 80,79,76,73,84,73,67,83,0,0,0,0,0,0  ' POLITICS
    DATA 83,80,79,82,84,83,0,0,0,0,0,0,0,0  ' SPORTS
