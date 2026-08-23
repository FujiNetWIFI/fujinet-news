' st_topics.bas -- topic-selection screen.
'
' Blue background, dark-green selection bar (MODE 0,1,4,1,4). Nine
' categories on rows 2-10, each direct-selectable with keypad 1-9 or via
' disc + Enter/action button. Menu row: keypad-key tokens in yellow,
' command words in white.

    DIM topic_sel, tp_i

topics_enter:
    MODE 0,CS_BLUE,CS_DARKGREEN,CS_BLUE,CS_DARKGREEN
    WAIT
    CLS
    list_valid = 0

    PRINT AT screenpos(4,0) COLOR CS_YELLOW,"FUJINET NEWS"

    FOR tp_i = 0 TO 8
        ' Keypad digit '1'+tp_i at column 2 (card = ascii-32 = 17+tp_i).
        #BACKTAB(screenpos(2, 2 + tp_i)) = (17 + tp_i) * 8 + CS_YELLOW
        s_row = 2 + tp_i
        s_col = 4
        s_max = 14
        s_col_color = CS_WHITE
        #s_src = VARPTR lit_titles(0) + tp_i * 14
        GOSUB scr_puts
    NEXT tp_i

    PRINT AT screenpos(0,11) COLOR CS_YELLOW,"1-9"
    PRINT COLOR CS_WHITE," "
    PRINT COLOR CS_YELLOW,"^V"
    PRINT COLOR CS_WHITE,":PICK "
    PRINT COLOR CS_YELLOW,"ENT"
    PRINT COLOR CS_WHITE,":READ"

    ' Selection bar last -- PRINT/scr_puts write whole words and would
    ' wipe the advance bits.
    GOSUB topics_bar_set

topics_loop:
    WAIT
    GOSUB in_poll
    IF in_disc = DISC_UP THEN GOTO topics_up
    IF in_disc = DISC_DOWN THEN GOTO topics_down
    IF in_btn = 1 THEN GOTO topics_select
    IF in_key = KEYPAD_ENTER THEN GOTO topics_select
    IF (in_key >= 1) AND (in_key <= 9) THEN topic_sel = in_key - 1 : GOTO topics_select
    GOTO topics_loop

topics_up:
    IF topic_sel > 0 THEN GOSUB hl_clear : topic_sel = topic_sel - 1 : GOSUB topics_bar_set
    GOTO topics_loop

topics_down:
    IF topic_sel < 8 THEN GOSUB hl_clear : topic_sel = topic_sel + 1 : GOSUB topics_bar_set
    GOTO topics_loop

topics_select:
    #list_page = 1
    list_sel = 0
    GOTO list_enter

topics_bar_set: PROCEDURE
    #hl_a = (2 + topic_sel) * 20
    #hl_b = (3 + topic_sel) * 20
    GOSUB hl_set
END
