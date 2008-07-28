all:
	(cd src; ./buildmake.sh; make clean; make depend; make)
	(cd test; make clean ; make)
	(cd utils; make clean ; make)
