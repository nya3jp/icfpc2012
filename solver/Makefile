CXX=g++
CXXFLAGS=-O2 -Wall -Werror -g -static

TARGETS=console simulator_test vita gasolver boulder_test simple_simulator interactive evaluator greedy_solver montecarlo_solver signum_standalone

.PHONY: all clean

all: $(TARGETS)

console.o: simulator.h
simulator_test.o: simulator.h
boulder_test.o: boulder.h
simple_simulator.o: boulder.h
interactive.o: boulder.h
evaluator.o: boulder.h
greedy_solver.o: boulder.h
solver_main.o: boulder.h
montecarlo_solver.o: boulder.h
gasolver.o: boulder.h
vita.o: boulder.h
signum.o: boulder.h
signum_standalone.o:  boulder.h signum.h

console: console.o simulator.o
	$(CXX) $(CXXFLAGS) -o $@ $^

xhlsolver: xhlsolver.o simulator.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# vita: xhlsolver4.o simulator.o
# 	$(CXX) $(CXXFLAGS) -o $@ $^

vita: vita4.o solver_main.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

signum_standalone: signum_standalone.o signum.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

simulator_test: simulator_test.o simulator.o
	$(CXX) $(CXXFLAGS) -o $@ $^

gasolver: gasolver.o signum.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

boulder_test: boulder_test.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

simple_simulator: simple_simulator.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

interactive: interactive.o boulder.o
	$(CXX) -O2 -Wall -Werror -g -o $@ $^ -lncurses

evaluator: evaluator.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

greedy_solver: greedy_solver.o solver_main.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

montecarlo_solver: montecarlo_solver.o boulder.o
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f *.o *~ $(TARGETS)
