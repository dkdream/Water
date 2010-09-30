##-*- mode: Makefile;-*-
#----------------------------------
#
# Project: Water
#
# CreatedBy: Thom Wood
# CreatedOn: May of 2010
#
#----------------------------------
#

TIME      := $(shell date +T=%s.%N)
TERM      := dump
COLORTERM := dump
HERE      := $(shell pwd)
DEPENDS   :=


##
COPPER     := /tools/Copper/bin/copper
COPPER_LIB := -L/tools/Copper/lib -lCopper
COPPER_INC := -I/tools/Copper/include
##
MERCURY     := /tools/Mercury/bin/mercury
MERCURY_INC := -I/tools/Mercury/include
MERCURY_LIB := -L/tools/Mercury/lib -lMercury
##
WATER     := ../water.vm
WATER_INC := -I../
WATER_LIB := -L../ -lWater
#
#
#
GCC      := gcc
DBFLAGS  := -ggdb -Wall -mtune=i686
INCFLAGS := $(WATER_INC) $(COPPER_INC)
CFLAGS   := $(DBFLAGS) $(INCFLAGS)
LIBFLAGS := $(WATER_LIB)

#
#
MAINS     := $(notdir $(wildcard test_*.c))
LANGS     := $(notdir $(wildcard *.h2o))
C_SOURCES := $(filter-out $(MAINS) $(LANGS:%.h2o=%.c),$(notdir $(wildcard *.c)))
H_SOURCES := $(notdir $(wildcard *.h))

#
#
all :: $(MAINS:%.c=%.x)

test ::

checkpoint : ; git checkpoint

$(RUNS) : $(WATER)

water_compile : $(LANGS:%.h2o=%.o)

clean ::
	@rm -rf .depends .tests
	@rm -f .*~ *~ *.x
	rm -f $(OBJS) $(ASMS) $(LANGS:%.h2o=%.o) $(LANGS:%.h2o=%.c) $(MAINS:%.c=%.o)

scrub :: 
	@make clean

# --

.PHONY :: all
.PHONY :: asm
.PHONY :: test
.PHONY :: install
.PHONY :: checkpoint
.PHONY :: clear
.PHONY :: clean
.PHONY :: scrub
.PHONY :: scrub
.PHONY :: tail.check

# -- keep intermediate
.SECONDARY :: $(LANGS:%.h2o=%.c)

##
## rules
##

%.c : %.h2o $(WATER)
	$(WATER) --name $(@:%.c=%_graph) --output $@ --file $<

%.o : %.c
	$(GCC) $(CFLAGS) -c -o $@ $<

%.x : %.o ; $(GCC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

#
#
#

.depends : ; @mkdir .depends

.depends/%.d : %.c .depends ; @$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<

#
#
#

-include $(DEPENDS)

.tests : ; @mkdir .tests

.tests/%.mk : test_%.c .tests ./test.rules ; @./test.rules $(@:.tests/%.mk=%) > $@

-include $(MAINS:test_%.c=./.tests/%.mk)


