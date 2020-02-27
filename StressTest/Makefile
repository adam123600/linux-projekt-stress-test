all: directory makeprograms linkprograms

directory:
	mkdir -p plikiWyjsciowe

makeprograms:
	$(MAKE) -C massivereader; $(MAKE) -C multiwriter

linkprograms:
	ln -s ../massivereader/massivereader plikiWyjsciowe/massivereader ; ln -s ../multiwriter/multiwriter plikiWyjsciowe/multiwriter

clean:
	$(MAKE) -C massivereader clean; $(MAKE) -C multiwriter clean; rm -rf plikiWyjsciowe
