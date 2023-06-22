target = jutt-a3
allFiles = Makefile a3w23.c Report.pdf

a3w23: a3w23.c 
	gcc a3w23.c -o a3w23

tar:
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

clean:
	rm -rf *
