# CppUTest imports
CPPUTEST_HOME = /opt/homebrew/opt/cpputest
CPPFLAGS += -I$(CPPUTEST_HOME)/include
CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
CFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorMallocMacros.h
LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt

# Object files for testing
TEST_OBJS := $(patsubst %.c, %_test.o, $(wildcard MPSPEnv/c/src/*.c))

# Object files for final build
BUILD_OBJS := $(patsubst %.c, %_build.o, $(wildcard MPSPEnv/c/src/*.c))

# Compile and run tests
cpputest: $(TEST_OBJS)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CFLAGS) -o unit_test MPSPEnv/c/tests/*.cpp $^ $(LD_LIBRARIES)
	@./unit_test -c

# Rule to make object files for testing
%_test.o: %.c %.h
	@gcc $(CFLAGS) -c -o $@ $<

# Create .so file for python bindings, but without CppUTest
build: $(BUILD_OBJS)
	@gcc -shared -o MPSPEnv/c_lib.so $^

# Rule to make object files for final build
%_build.o: %.c %.h
	@gcc -fPIC -c $< -o $@

test:
	@/opt/homebrew/bin/python3.11 -m pytest -q tests/ -W ignore::DeprecationWarning

clean:
	@rm -f MPSPEnv/c/src/*.o
	@rm -f MPSPEnv/c_lib.so
	@rm -f unit_test