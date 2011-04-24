all:
	(cd src; ./buildmake.sh; make clean; make depend; make)
	(cd test; make clean ; make)
	(cd utils; make clean ; make)

clean:
	(cd src ; ./buildmake.sh ; make clean)
	(cd test ; make clean )
	(cd utils ; make clean )
