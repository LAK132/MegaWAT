.export     _font_load_asm

_font_load_asm:
    ; a-reg contains length
    .byte $4B       ; taz
    nop
    lda #$2e
    ; string in memory location 0x0700
    ldx #$00
    ldy #$07
    sta $d640
    nop
    .byte $a3,$00   ; ldz #$00
    nop

    ; close all files to work around kickstart file descriptor leak bug
    lda #$22
    sta $d640
    nop

    ; carry set on success
    ; carry clear on failure
    bcs _font_load_load
    lda #$01
    rts

_font_load_load:
    ; name set, now load font
    lda #$36
    ldx #$00
    ldy #$00
    .byte $a3,$04   ; ldz #$04
    nop
    sta $d640
    nop
    .byte $a3,$00   ; ldz #$00
    nop

    ; close all files to work around kickstart file descriptor leak bug
    lda #$22
    sta $d640
    nop

    ; carry set on success
    ; carry clear on failure
    bcs _font_load_done
    lda #$02
    rts

_font_load_done:
    lda #$00
    rts