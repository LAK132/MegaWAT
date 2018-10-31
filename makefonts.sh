#!/bin/csh

set ttftof65=c65gs-font-rasteriser/ttftof65

# Normal fonts
foreach size (8 14 22 30)
foreach font (`echo assets/*.ttf assets/*.otf`)
set fontname=`echo $font | cut -f1 -d.`
echo ${ttftof65} -T ${font} -A -P ${size} -o ${fontname}-${size}.f65
${ttftof65} -T ${font} -A -P ${size} -o ${fontname}-${size}.f65
end
end

#bigger capitals only typeface for headings
set size=54
foreach font (`echo assets/*.ttf assets/*.otf`)
set fontname=`echo $font | cut -f1 -d.`
echo ${ttftof65} -T ${font} -8 "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890\!.,?" -P ${size} -o ${fontname}-${size}.f65
${ttftof65} -T ${font} -A -P ${size} -o ${fontname}-${size}.f65
end

# Now make a pack of them that fits in 128KB
set fontlist=( \
    assets/UbuntuMono-R-14.f65 \
    assets/caladea-regular-14.f65 \
    assets/Entypo-22.f65 \
    assets/caladea-regular-22.f65 \
    assets/caladea-regular-30.f65 \
    )

set size=`ls -l $fontlist | cut -f5 -d" " | awk '{ n+=$1; } END { print n; }'`
echo "This will take $size bytes"
if ( $size > 131071 ) then
  echo "ERROR: Fonts must be <128KB in total"
  exit 1
endif

rm $1

foreach typeface ( $fontlist )
ls -l $typeface
dd if=${typeface} bs=1 skip=2 of=$1 conv=notrunc oflag=append
end
# Put end marker (empty 256 byte block) on
dd if=/dev/zero bs=256 count=1 of=fontpack.bin conv=notrunc oflag=append


ls -l $1
