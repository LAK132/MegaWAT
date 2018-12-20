TTFTOF65 = ../c65gs-font-rasteriser/ttftof65

define F65_TEMPLATE =
$(1)-A-%.f65: $(1).$(2) Makefile
	$$(TTFTOF65) -T $$< -P $$* -o $$@ -A
$(1)-8-%.f65: $(1).$(2) Makefile
	$$(TTFTOF65) -T $$< -P $$* -o $$@ -8 $(3)
endef

define FPK_TEMPLATE =
$(1).FPK: $(2)
	$(foreach F65,$(2),dd if=$(F65) bs=1 skip=2 of=$$@ conv=notrunc oflag=append;) dd if=/dev/zero bs=256 count=1 of=$$@ conv=notrunc oflag=append
endef

$(eval $(call F65_TEMPLATE,Entypo,otf,""))
$(eval $(call F65_TEMPLATE,caladea-regular,ttf,""))
$(eval $(call F65_TEMPLATE,caladea-bold,ttf,""))
$(eval $(call F65_TEMPLATE,caladea-bolditalic,ttf,""))
$(eval $(call F65_TEMPLATE,caladea-italic,ttf,""))
$(eval $(call F65_TEMPLATE,ComicRelief,ttf,""))
$(eval $(call F65_TEMPLATE,ComicRelief-Bold,ttf,""))
$(eval $(call F65_TEMPLATE,example,ttf,""))
$(eval $(call F65_TEMPLATE,GlacialIndifference-Regular,otf,"MEGAegaWT65!?"))
$(eval $(call F65_TEMPLATE,GlacialIndifference-Bold,otf,"MEGAegaWT65!?"))
$(eval $(call F65_TEMPLATE,GlacialIndifference-Italic,otf,"MEGAegaWT65!?"))
$(eval $(call F65_TEMPLATE,heydings_icons,ttf,""))
$(eval $(call F65_TEMPLATE,UbuntuMono-R,ttf,""))

$(eval $(call FPK_TEMPLATE,DEFAULT,caladea-regular-A-14.f65 caladea-regular-A-22.f65 caladea-regular-A-30.f65 Entypo-A-22.f65 UbuntuMono-R-A-14.f65))
$(eval $(call FPK_TEMPLATE,TITLE,GlacialIndifference-Bold-8-16.f65))

.DEFAULT:
	$(error Could not find file $@)
