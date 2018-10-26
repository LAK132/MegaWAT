.export _lpoke_asm
.export _lpeek_asm
.import _fast_memory_address
.import _fast_memory_value

_lpoke_asm:
    lda <_fast_memory_value
    .byte $a3,$00 ; = ldz #$00
    nop
    .byte $92,<_fast_memory_address ; = sta (_fast_memory_address),z
    rts

_lpeek_asm:
    .byte $a3,00
    nop
    .byte $b2,<_fast_memory_address ; = lda (_fast_memory_address),z
    sta _fast_memory_value
    rts