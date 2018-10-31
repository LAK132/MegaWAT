The planned memory layout is:

$0000000 - $00003FF - ZP, stack etc as usual
$0000400 - $00007FF - Normal C64 screen
$0000800 - $000CFFF - Code (normal C64-mode loading program) (50KB)
$000D000 - $000DFFF - C64 charset / IO as normal. Not currently using the 4K RAM beneath.
$000E000 - $000FFFF - C64 KERNAL ROM
$0010000 - $0011FFF - C65 DOS RESERVED
$0012000 - $0014FFF - Slide #0 screen RAM (100x60x 2 bytes per tile)
$0015000 - $0017FFF - Slide #1 (next) screen RAM
$0018000 - $001AFFF - Slide #2 (previous) screen RAM
$001B000 - $001DFFF - Slide #2 (previous) colour RAM
$001E000 - $001F7FF - Text render buffer for screen memory (100 x 30 tiles x 2 bytes per tile)
$0020000 - $0023FFF - C65 DOS ROM
$0024000 - $0027FFF - Slide data (16KB) [editor text buffer]
$0028000 - $002FFFF - C64 and C65 ROMs
$0030000 - $003BFFF - C65 ROMs
$003C000 - $003DFFF - More slide data (8KB) [editor single line text buffer]
$003E000 - $003FFFF - C65 ROMs
$0040000 - $005FFFF - Fonts and graphics (as fonts) etc
$FF80000 - $FF807FF - C64/C65 colour RAM
$FF80800 - $FF837FF - Slide #0 colour RAM
$FF83800 - $FF867FF - Slide #1 (next) colour RAM
$FF86800 - $FF87FFF - Text render rbuffer for colour memory (100 x 30 tiles x 2 bytes per tile)