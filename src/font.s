    .export _font_file
    .export _font_file_size
_font_file:  .word font_data+2
_font_file_size:  .word after_font_data-(font_data+2)
font_data:
    .incbin "example.f65"
after_font_data:
