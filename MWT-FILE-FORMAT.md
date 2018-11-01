# Overview
MWT files begin with `^A`/`$01` (Start of Heading), followed by an offset to the font data (end of slide data), then the slides and fonts.

`^A[32bit offset to fonts][slide texts][font datas]`

# Slide text
`^@` / `$00` (Null): marks the end of slide data

`^J` / `$0A` (Line Feed): ends the current line

`^L` / `$0C` (Form Feed): seperates slides

`^[[0m` / `^[[m` / `$1B $9B $30 $6D` / `$1B $9B $6D` (SGR Reset): reset all attributes

`^[[4m` / `$1B $9B $34 $6D` (Underline): enable hardware underline

`^[[24m` / `$1B $9B $32 $34 $6D` (Underline Off): disable hardware underline

`^[[5m` / `$1B $9B $35 $6D` (Slow Blink): enable hardware blink

`^[[25m` / `$1B $9B $32 $35 $6D` (Blink Off): disable hardware blink

`^[[7m` / `$1B $9B $37 $6D` (Reverse Video): enable hardware colour reverse

`^[[27m` / `$1B $9B $32 $37 $6D` (Reverse Off): disable hardware colour reverse

`^[[1nm` / `$1B $9B $31 $3n $6D` (Alternative Font): selects font `n`

`^[[3nm` / `$1B $9B $33 $3n $6D` (Foreground Colour): selects low-colour `n`

`^[[9nm` / `$1B $9B $39 $3n $6D` (Bright Foreground Colour): selects high-colour `n`

`^[[38m` / `$1B $9B $33 $38 $6D` (Default Foreground Colour): selects white on MEGA65

`^[[mSlide1^J^[[mHello^J^[[mWorld^J^L^[[mSlide2^J^@` would render as

```
Slide1
Hello
World
```
```
Slide 2
```

# Colour map

MEGA65 -> Unicode
```
MEGA65          Unicode
0x0 (Blk)       "30" (Black)
0x1 (Wht)       "97" (B White)
0x2 (Red)       "31" (Red)
0x3 (Cyn)       "36" (Cyan)
0x4 (Pur)       "35" (Magenta)
0x5 (Grn)       "32" (Green)
0x6 (Blu)       "34" (Blue)
0x7 (Yel)       "93" (B Yellow)

0x8 (Orng)      "33" (Yellow)
0x9 (Brn)       "95" (B Magenta)
0xA (L Red)     "91" (B Red)
0xB (D Gry)     "90" (B Black)
0xC (M Gry)     "96" (B Cyan)
0xD (L Grn)     "92" (B Green)
0xE (L Blu)     "94" (B Blue)
0xF (L Gry)     "37" (White)
```
Unicode -> MEGA65
```
Unicode             MEGA65
"30" (Black)        0x0 (Blk)
"31" (Red)          0x2 (Red)
"32" (Green)        0x5 (Grn)
"33" (Yellow)       0x8 (Orng)
"34" (Blue)         0x6 (Blu)
"35" (Magenta)      0x4 (Pur)
"36" (Cyan)         0x3 (Cyn)
"37" (White)        0xF (L Gry)

"90" (B Black)      0xB (D Gry)
"91" (B Red)        0xA (L Red)
"92" (B Green)      0xD (L Grn)
"93" (B Yellow)     0x7 (Yel)
"94" (B Blue)       0xE (L Blu)
"95" (B Magenta)    0x9 (Brn)
"96" (B Cyan)       0xC (M Gry)
"97" (B White)      0x1 (Wht)
```