CC    ?= gcc
STRIP ?= strip
AR    ?= ar

ifneq ($(OS_NAME),)
	TARGET_OS = $(OS_NAME)
else
	ifeq ($(OS),Windows_NT)
		TARGET_OS = Windows
	else ifeq ($(PLATFORM),iPhoneOS)
		TARGET_OS = iPhoneOS
	else ifeq ($(PLATFORM),iPhoneSimulator)
		TARGET_OS = iPhoneSimulator
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Darwin)
			TARGET_OS = Mac
		else
			TARGET_OS = Linux
		endif
	endif
endif

ifeq ($(TARGET_OS),Windows)
	#CFLAGS   =
	#LFLAGS   =
	IMPLIB   = sqlite3
	LIBRARY  = sqlite3.dll
	WINLIB   := $(IMPLIB).lib
	override LDFLAGS  += -static-libgcc -static-libstdc++
else ifeq ($(TARGET_OS),iPhoneOS)
	LIBRARY = libsqlite3.dylib
	CFLAGS += -fPIC
else ifeq ($(TARGET_OS),iPhoneSimulator)
	LIBRARY = libsqlite3.dylib
	CFLAGS += -fPIC
else
	ifeq ($(TARGET_OS),Mac)
		OS = OSX
		LIBRARY  = libsqlite3.0.dylib
		LIBNICK1 = libsqlite3.dylib
		INSTNAME = $(LIBPATH)/libsqlite3.dylib
		CURR_VERSION   = 9.0.0
		COMPAT_VERSION = 9.0.0
	else
		LIBRARY  = libsqlite3.so.0.0.1
		LIBNICK1 = libsqlite3.so.0
		LIBNICK2 = libsqlite3.so
		SONAME   = libsqlite3.so.0
	endif
	IMPLIB   = sqlite3
	prefix  ?= /usr/local
	LIBPATH  = $(prefix)/lib
	INCPATH  = $(prefix)/include
	EXEPATH  = $(prefix)/bin
	LIBFLAGS += -fPIC
	override LDFLAGS  += -lpthread
	DEBUGFLAGS = -rdynamic
	SHELLFLAGS = -DHAVE_READLINE
endif

SHORT = sqlite3

# the item below cannot be called SHELL because it's a reserved name
ifeq ($(TARGET_OS),Windows)
	SSHELL = sqlite3.exe
else
	SSHELL = sqlite3
endif

LIBFLAGS := $(LIBFLAGS) $(CFLAGS) -DSQLITE_USE_URI=1 -DSQLITE_ENABLE_JSON1 -DSQLITE_THREADSAFE=1 -DHAVE_USLEEP -DSQLITE_ENABLE_COLUMN_METADATA


.PHONY:  install debug test tests clean valgrind sanitizer


all:      $(LIBRARY) $(SSHELL)

debug:    $(LIBRARY) $(SSHELL)

ios:      lib$(SHORT).a lib$(SHORT).dylib

#debug:   export LIBFLAGS := -g -DSQLITE_DEBUG=1 -DDEBUGPRINT $(DEBUGFLAGS) $(LIBFLAGS)
debug:    export LIBFLAGS := -g -DSQLITE_DEBUG $(DEBUGFLAGS) $(LIBFLAGS)

valgrind: export LIBFLAGS := -g -DSQLITE_DEBUG $(DEBUGFLAGS) $(LIBFLAGS)


# Windows
$(SHORT).dll: $(SHORT).o
	$(CC) -shared -Wl,--out-implib,$(IMPLIB).lib $^ -o $@ $(LFLAGS)
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) $@
endif

# OSX
lib$(SHORT).0.dylib: $(SHORT).o
	$(CC) -dynamiclib -install_name "$(INSTNAME)" -current_version $(CURR_VERSION) -compatibility_version $(COMPAT_VERSION) $^ -o $@ $(LDFLAGS)
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) -x $@
endif
	ln -sf $(LIBRARY) $(LIBNICK1)

# iOS
lib$(SHORT).dylib: $(SHORT).o
	$(CC) -dynamiclib -o $@ $^ $(LDFLAGS)
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) -x $@
endif

# Linux / Unix
lib$(SHORT).so.0.0.1: $(SHORT).o
	$(CC) -shared -Wl,-soname,$(SONAME) $^ -o $@ -ldl $(LDFLAGS)
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) $@
endif
	ln -sf $(LIBRARY) $(LIBNICK1)
	ln -sf $(LIBNICK1) $(LIBNICK2)

$(SHORT).o: $(SHORT).c $(SHORT).h
	$(CC) -c $(LIBFLAGS) $< -o $@

$(SSHELL): shell.o $(LIBRARY)
ifeq ($(TARGET_OS),Windows)
	$(CC) $< -o $@ -L. -l$(IMPLIB) $(LFLAGS)
else ifeq ($(TARGET_OS),Mac)
	$(CC) $< -o $@ -L. -lsqlite3 -ldl -lreadline
else
	$(CC) $< -o $@ -Wl,-rpath,$(LIBPATH) -L. -lsqlite3 -ldl -lreadline
endif
	strip $(SSHELL)

shell.o: shell.c
	$(CC) -c $(SHELLFLAGS) $< -o $@

install:
	mkdir -p $(INCPATH)
	mkdir -p $(EXEPATH)
	mkdir -p $(LIBPATH)
	cp $(LIBRARY) $(LIBPATH)/
	cd $(LIBPATH) && ln -sf $(LIBRARY) $(LIBNICK1)
ifeq ($(TARGET_OS),Mac)
else
	cd $(LIBPATH) && ln -sf $(LIBNICK1) $(LIBNICK2)
endif
	cp sqlite3.h $(INCPATH)/
	cp sqlite3ext.h $(INCPATH)/
	cp $(SSHELL) $(EXEPATH)/
ifeq ($(TARGET_OS),Linux)
	ldconfig
endif

clean:
	rm -f *.o lib$(SHORT).a lib$(SHORT).dylib $(LIBRARY) $(LIBNICK1) $(LIBNICK2) $(SSHELL) test/runtest

test: $(LIBRARY) test/runtest
ifeq ($(TARGET_OS),Mac)
	cd test && DYLD_LIBRARY_PATH=..:/usr/local/lib ./runtest
else
	cd test && LD_LIBRARY_PATH=..:/usr/local/lib ./runtest
endif

valgrind: $(LIBRARY) test/runtest
ifeq ($(TARGET_OS),Mac)
	cd test && DYLD_LIBRARY_PATH=..:/usr/local/lib valgrind --leak-check=full --show-leak-kinds=all ./runtest
else
	cd test && LD_LIBRARY_PATH=..:/usr/local/lib valgrind --leak-check=full --show-leak-kinds=all ./runtest
endif

sanitizer:
	gcc -fsanitize=address sqlite3.c test/test.c -pthread -ldl -lpthread -o test-with-sanitizer
	./test-with-sanitizer
	rm test-with-sanitizer

test/runtest: test/test.c
	$(CC) $< -o $@ -I. -L. -lsqlite3

test2: test/test.py
ifeq ($(TARGET_OS),Windows)
ifeq ($(PY_HOME),)
	@echo "PY_HOME is not set"
else
	cd $(PY_HOME)/DLLs && [ ! -f sqlite3-orig.dll ] && mv sqlite3.dll sqlite3-orig.dll || true
	cp $(LIBRARY) $(PY_HOME)/DLLs/sqlite3.dll
	cd test && python test.py -v
endif
else
ifeq ($(OS),OSX)
ifneq ($(shell python -c "import pysqlite2.dbapi2" 2> /dev/null; echo $$?),0)
ifneq ($(shell [ -d $(LIBPATH2) ]; echo $$?),0)
	@echo "run 'sudo make install' first"
endif
	git clone --depth=1 https://github.com/ghaering/pysqlite
	cd pysqlite && echo "include_dirs=$(INCPATH)" >> setup.cfg
	cd pysqlite && echo "library_dirs=$(LIBPATH2)" >> setup.cfg
	cd pysqlite && python setup.py build
	cd pysqlite && sudo python setup.py install
endif
	cd test && python test.py -v
else	# Linux
	cd test && LD_LIBRARY_PATH=..:/usr/local/lib python test.py -v
endif
endif


# variables:
#   $@  output
#   $^  all the requirements
#   $<  first requirement
