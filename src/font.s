    .export _font_file
    .export _font_size
_font_file:  .word font_data
_font_size:  .word after_font_data-font_data
font_data:
    .incbin "example.f65"
after_font_data: