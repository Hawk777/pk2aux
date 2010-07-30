LIB_OBJS := rw.o scan.o id.o power.o sigpins.o uart.o
LIB_OUT := lib/libpk2aux.a

# Clean by removing all object modules plus the library file.
clean-lib:
	-rm -f ${LIB_OBJS:%=lib/%}
	-rm -f ${LIB_OUT}

# Build the library file by depending on all the object modules using
# archive member syntax, and then running ranlib at the end.
lib/libpk2aux.a: ${LIB_OBJS:%.o=${LIB_OUT}(lib/%.o)}
	ranlib $@

# Each object module depends on its own source file (by the implicit rule)
# and also all the header files.
${LIB_OBJS:%.o=lib/%.o}: lib/cmd.h lib/internal.h lib/include/pk2aux.h

.PHONY: clean-lib
