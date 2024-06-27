# makefile

.PYHONY: all clean

all: template.out basic_usage.out example0.out example1.out optional_type.out optional_type2.out

template.out: template.cpp
	g++ -std=c++20 -o template.out template.cpp

basic_usage.out: basic_usage.cpp
	g++ -std=c++20 -o basic_usage.out basic_usage.cpp

example0.out: example0.cpp
	g++ -std=c++20 -o example0.out example0.cpp

example1.out: example1.cpp
	g++ -std=c++20 -o example1.out example1.cpp

optional_type.out: optional_type.cpp
	g++ -std=c++20 -o optional_type.out optional_type.cpp

optional_type2.out: optional_type2.cpp
	g++ -std=c++20 -o optional_type2.out optional_type2.cpp

clean:
	rm -f *.out
