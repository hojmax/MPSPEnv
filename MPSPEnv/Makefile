# CppUTest imports
CPPUTEST_HOME = /opt/homebrew/opt/cpputest
CPPFLAGS += -I$(CPPUTEST_HOME)/include
CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
CFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorMallocMacros.h
LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt

# Object files for testing
TEST_OBJS := $(patsubst %.c, %_test.o, $(wildcard c/src/*.c))

# Object files for final build
BUILD_OBJS := $(patsubst %.c, %_build.o, $(wildcard c/src/*.c))

# Coverage flags
COVERAGE_FLAGS = -fprofile-arcs -ftest-coverage

# Compile and run tests
test: $(TEST_OBJS)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -o unit_test c/tests/*.cpp $^ $(LD_LIBRARIES)
	@./unit_test -c
	@make clean

# Rule to make object files for testing
%_test.o: %.c %.h
	@gcc $(CFLAGS) $(COVERAGE_FLAGS) -c -o $@ $<

# Create .so file for python bindings, but without CppUTest
build: $(BUILD_OBJS)
	@gcc -shared -O3 -DNDEBUG -o c_lib.so $^

# Rule to make object files for final build
%_build.o: %.c %.h
	@gcc -c -O3 -DNDEBUG $< -o $@

# Generate Coverage Report
coverage:
	@gcovr -r . --html --html-details -o coverage_report.html

train:
	@python rl/main.py

episode: build
	@gcc -o run_episodes c/tests/run_episodes.c c_lib.so
	./run_episodes

clean:
	@rm -f c/src/*.o
	@rm -f c/src/*.gcda
	@rm -f c/src/*.gcno
	@rm -f unit_test
	@rm -f -r .vscode
	@rm -f .DS_Store
	@rm -f c_lib.so
	@rm -f -r __pycache__
	@rm -f *.gcda
	@rm -f *.gcno
	@rm -f coverage_report*
	@rm -f run_episodes