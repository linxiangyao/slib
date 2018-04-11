testall: testslib client server

testslib: slib
	g++ ../../test/testS.cpp \
		-std=c++11 -L. -lslib -lpthread -ggdb -ldl -o testslib

client: slib
	g++ ../../test/testS_client.cpp \
     	-std=c++11 -L. -lslib -lpthread -ldl -ggdb -o client

server: slib
	g++ ../../test/testS_server.cpp \
     	-std=c++11 -L. -lslib -lpthread -ldl -ggdb -o server

slib:
	gcc ../../src/slib.c -c -o slibc.o
	g++ ../../src/slib.cpp -c -std=c++11 -o slibcpp.o
	ar crv libslib.a slibcpp.o slibc.o

clean:
	rm -f *.a
	rm -f *.o
	rm -f *.so
	rm -f *.out
	rm -f *.db
	rm -f *.log
	rm -f *.db
	rm -rf *.dSYM
