##############MAKEFILE#################
lib:
    g++ -c tinyxml.cpp -O
    g++ -c tinyxmlparser.cpp -O
    g++ -c tinyxmlerror.cpp -O
    g++ -c tinystr.cpp -O
    g++ -c xmlparse.cpp -O
    ar -rc libxml.a tinyxml.o tinyxmlparser.o tinyxmlerror.o tinystr.o xmlparse.o
    rm -rf *.o
debug:
    g++ -c tinyxml.cpp -O -DDebug
    g++ -c tinyxmlparser.cpp -O -DDebug
    g++ -c tinyxmlerror.cpp -O -DDebug
    g++ -c tinystr.cpp -O -DDebug
    g++ -c xmlparse.cpp -O -DDebug
    ar -rc libxml.a tinyxml.o tinyxmlparser.o tinyxmlerror.o tinystr.o xmlparse.o
    rm -rf *.o
clean:
    rm -rf *.o libxml.a
