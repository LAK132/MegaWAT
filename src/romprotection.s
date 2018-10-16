.export _toggle_write_protection

_toggle_write_protection:
    lda #$70
    sta $D640
    nop
    rts