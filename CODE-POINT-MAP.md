`$0000-$001F`: Unicode control characters

`$0000`: MegaWAT!? new line (Null) (export as `$000A`)

`$0009`: MegaWAT!? tab (Character Tabulation)

`$000C`: MegaWAT!? slide separator (Form Feed)

`$001C`: MegaWAT!? presentation separator (File Separator)

---

`$0020-$257F`: Unicode text

`$2580-$25FF`: Unicode shapes

`$2600-$D7FF`: Unicode text

`$D800-$DFFF`: Unicode surrogate pairs

---

`$E0?X`: MegaWAT!? colour selection

Unicode: `$1B $9B $[3X/9X] $6D`

---

`$E0X?`: MegaWAT!? attribute selection

Unicode: `$1B $9B $[04/24] $6D` for underline/no underline

Unicode: `$1B $9B $[05/25] $6D` for blink/no blink

Unicode: `$1B $9B $[07/27] $6D` for reverse/no reverse

---

`$E1XX`: MegaWAT!? font selection

Unicode: `$1B $9B $(XX-A) $6D`

`$F800-$FFFF`: Standard unicode text