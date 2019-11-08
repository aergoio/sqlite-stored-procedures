# The SONAME sets the api version compatibility.
# It is using the same SONAME from the pre-installed sqlite3 library so
# the library can be loaded by existing applications as python. For this
# we can set the LD_LIBRARY_PATH when opening the app or set the rpath
# in the executable.

ifeq ($(OS),Windows_NT)
    #CFLAGS   =
    #LFLAGS   =
    IMPLIB   = sqlite3
    LIBRARY  = sqlite3.dll
    LDFLAGS  += -static-libgcc -static-libstdc++
else ifeq ($(PLATFORM),iPhoneOS)
    LIBRARY = libsqlite3.dylib
    CFLAGS += -fPIC
else ifeq ($(PLATFORM),iPhoneSimulator)
    LIBRARY = libsqlite3.dylib
    CFLAGS += -fPIC
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        OS = OSX
        LIBRARY  = libsqlite3.0.dylib
        LIBNICK1 = libsqlite3.dylib
        INSTNAME = $(LIBPATH)/libsqlite3.dylib
        CURR_VERSION   = 3.27.2
        COMPAT_VERSION = 3.0.0
        prefix  ?= /usr/local
    else
        #IMPLIB   = sqlite3
        LIBRARY  = libsqlite3.so.0.0.1
        LIBNICK1 = libsqlite3.so.0
        LIBNICK2 = libsqlite3.so
        SONAME   = libsqlite3.so.0
        prefix  ?= /usr/local
    endif
    LIBPATH  = $(prefix)/lib
    INCPATH  = $(prefix)/include
    EXEPATH  = $(prefix)/bin
    LIBFLAGS += -fPIC
    LDFLAGS  += -lpthread
    DEBUGFLAGS = -rdynamic
    SHELLFLAGS = -DHAVE_READLINE
endif

CC    = gcc
STRIP = strip
AR    = ar

SHORT = sqlite3

# the item below cannot be called SHELL because it's a reserved name
ifeq ($(OS),Windows_NT)
    SSHELL = sqlite3.exe
else
    SSHELL = sqlite3
endif

LIBFLAGS := $(LIBFLAGS) $(CFLAGS) -DSQLITE_USE_URI=1 -DSQLITE_ENABLE_JSON1 -DSQLITE_THREADSAFE=1 -DHAVE_USLEEP -DSQLITE_ENABLE_COLUMN_METADATA


.PHONY:  install debug test tests clean


all:     $(LIBRARY) $(SSHELL)

debug:   $(LIBRARY) $(SSHELL)

ios:     lib$(SHORT).a lib$(SHORT).dylib

debug:   export LIBFLAGS := -g -DSQLITE_DEBUG=1 -DDEBUGPRINT $(DEBUGFLAGS) $(LIBFLAGS)


$(SHORT).dll: $(SHORT).o
	$(CC) -shared -Wl,--out-implib,$(IMPLIB).lib $^ -o $@ $(LFLAGS)
	$(STRIP) $@

lib$(SHORT).0.dylib: $(SHORT).o
	$(CC) -dynamiclib -install_name "$(INSTNAME)" -current_version $(CURR_VERSION) -compatibility_version $(COMPAT_VERSION) $^ -o $@ $(LDFLAGS)
	$(STRIP) -x $@
	ln -sf $(LIBRARY) $(LIBNICK1)

lib$(SHORT).a: $(SHORT).o
	$(AR) rcs $@ $^

lib$(SHORT).dylib: $(SHORT).o
	$(CC) -dynamiclib -o $@ $^ $(LDFLAGS)
	$(STRIP) -x $@

lib$(SHORT).so.0.0.1: $(SHORT).o
	$(CC) -shared -Wl,-soname,$(SONAME) $^ -o $@ $(LDFLAGS)
	$(STRIP) $@
	ln -sf $(LIBRARY) $(LIBNICK1)
	ln -sf $(LIBNICK1) $(LIBNICK2)

$(SHORT).o: $(SHORT).c $(SHORT).h
	$(CC) $(LIBFLAGS) -c $< -o $@

$(SSHELL): shell.o $(LIBRARY)
ifeq ($(OS),Windows_NT)
	$(CC) $< -o $@ -L. -l$(IMPLIB) $(LFLAGS)
else ifeq ($(OS),OSX)
	$(CC) $< -o $@ -L. -lsqlite3 -ldl -lreadline
else
	$(CC) $< -o $@ -Wl,-rpath,$(LIBPATH) -L. -lsqlite3 -ldl -lreadline
endif
	strip $(SSHELL)

shell.o: shell.c
	$(CC) -c $(SHELLFLAGS) $< -o $@

install:
	mkdir -p $(LIBPATH)
	cp $(LIBRARY) $(LIBPATH)
	cd $(LIBPATH) && ln -sf $(LIBRARY) $(LIBNICK1)
ifeq ($(OS),OSX)
else
	cd $(LIBPATH) && ln -sf $(LIBNICK1) $(LIBNICK2)
endif
	cp $(SHORT).h $(INCPATH)
	cp $(SSHELL) $(EXEPATH)

clean:
	rm -f *.o lib$(SHORT).a lib$(SHORT).dylib $(LIBRARY) $(LIBNICK1) $(LIBNICK2) $(SSHELL)

test: test/runtest
	cd test && LD_LIBRARY_PATH=.. ./runtest

test/runtest: test/test.c
	$(CC) $< -o $@ -L. -lsqlite3

test2: test/test.py
ifeq ($(OS),Windows_NT)
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
