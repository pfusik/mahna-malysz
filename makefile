run: mama_hsc.xex
	ls -l $<
	cygstart $<

mama_hsc.xex: gra.xex
	xebin p -a -15391 -o $@ $<

gra.xex: gra.asx tlo.fnt tlo.scr sprajty.asx datamatrix.asx
	xasm /p /q /o:$@ $<

tlo.fnt tlo.scr: gr2logoser.exe tlo.png
	./gr2logoser.exe

sprajty.asx: sprajty.exe anime.png
	./sprajty.exe >$@

%.exe: %.c
	gcc -s -O2 -o $@ $< -lpng -lz

clean:
	rm -f gra.xex tlo.fnt tlo.scr logosed.png sprajty.asx gr2logoser.exe sprajty.exe

.DELETE_ON_ERROR:
