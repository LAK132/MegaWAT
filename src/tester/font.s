    global font_file
    global font_file_size
font_file: dd font_data
font_file_size: dd after_font_data-font_data
font_data:
    incbin "../../example.f65",2
after_font_data: