Catlib Libraries

BACKGROUND

These are libraries of C code that I have accumulated over time to
create a solid implementation of various data structures and algorithms
with some constraints based on my own whims:

 - The code in each module should be as independent as possible.  One
   should be able to build and run said code without having to include the
   rest of the library.  (there are a few modules that are notable exceptions
   to this.  stduse.h is a big one.)

 - The data structures should not generally not rely on malloc or other
   automatic memory management.  (at least this was the case until I wrote
   my own malloc()s in this library).  Data structures should be agnostic
   to memory allocation so they can be used with maximum flexibility.  In
   some cases my own API for abstracting dynamic memory management is used
   where dynamic allocation is all but unavoidable.  

 - Standard C types should be used except where fixed width types make
   absolute sense (e.g. bit operations).

 - Except for networking code, the code should depend on no external library
   calls except for the standard C library.  If the code will use an
   external API (again, networking code aside), then I should have my own
   version of it in the standard C library replacement.

 - No C99 -- only C89.


BUILDING

Linux:
------

To build on a linux system hopefully one can just type:

  make

at the top level directory.  


BSD:
----

I have had problems on BSD with this because their version of 'make'
seems to want to interpret certain directories (e.g. 'obj') in special
ways.  (For the record I don't want my build process to depend on either
gmake or pmake or any specific version of 'make' and DEFINITELY not
autotools.)  For now do:

  cd src
  ./buildmake.sh
  make clean ; make depend ; make 

  cd ../test
  make clean ; make

  cd ../utils
  make clean ; make

That is all that the top level Makefile does anyways.


Other OSes:
-----------

Well, I want this build system to be as portable as possible.  So email
me and I'll try to help.


AUTHOR

Christopher Adam Telfer (ctelfer@gmail.com)
