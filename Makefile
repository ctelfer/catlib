all:
	(cd src; make veryclean; make)
	(cd test; make clean ; make)
	(cd utils; make clean ; make)

clean:
	(cd src ; make veryclean)
	(cd test ; make clean )
	(cd utils ; make clean )
