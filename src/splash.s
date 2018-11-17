        .export _splashlogo

_splashlogo:       .word splashlogo_data
        
splashlogo_data:
        .incbin     "assets/megawat-splash.m65"

