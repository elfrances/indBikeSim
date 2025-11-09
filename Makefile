###########################################################################
#
#   Filename:           Makefile
#
#   Author:             Marcelo Mourier
#   Created:            Sat Nov  8 05:16:44 PM PST 2025
#
#   Description:        This makefile is used to build the indBikeSim
#                       command-line tool.
#
###########################################################################
#
#                  Copyright (c) 2025 Marcelo Mourier
#
###########################################################################

BIN_DIR = .
DEP_DIR = .
OBJ_DIR = .

OS := $(shell uname -o)

CFLAGS = -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0
LDFLAGS = -ggdb 

ifeq ($(OS),Cygwin)
	CFLAGS += -D__CYGWIN__
endif

SOURCES = $(wildcard *.c)
OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS := $(patsubst %.c,$(DEP_DIR)/%.d,$(SOURCES))

# Rule to autogenerate dependencies files
$(DEP_DIR)/%.d: %.c
	@set -e; $(RM) $@; \
         $(CC) -MM $(CPPFLAGS) $< > $@.temp; \
         sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)\/\1.o $@ : ,g' < $@.temp > $@; \
         $(RM) $@.temp

# Rule to generate object files
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

all: indBikeSim

indBikeSim: $(OBJECTS) Makefile
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/$@ $(OBJECTS) -lm -lreadline

clean:
	$(RM) $(OBJECTS) $(DEP_DIR)/*.d $(BIN_DIR)/indBikeSim

include $(DEPS)

