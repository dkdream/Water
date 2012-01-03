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

##
COPPER     := /tools/Copper/bin/copper
COPPER_INC := -I/tools/Copper/include
COPPER_LIB := -L/tools/Copper/lib -lCopper

##
WATER     := ../water.vm
WATER_INC := -I/tools/Water/include
WATER_LIB := -L/tools/Water/lib -lWater
#
#
#
GCC      := gcc
DBFLAGS  := -ggdb -Wall -W -mtune=i686
INCFLAGS := $(WATER_INC) $(COPPER_INC)
CFLAGS   := $(DBFLAGS) $(INCFLAGS)
LIBFLAGS := $(WATER_LIB) $(COPPER_LIB)

#
#
MAINS        := $(notdir $(wildcard test_*.c))
WATER_TREES  := $(notdir $(wildcard *.h2o))
COPPER_TREES := $(notdir $(wildcard *.cu))
#
GENERATED_C  := $(WATER_TREES:%.h2o=%.c)
GENERATED_C  += $(COPPER_TREES:%.cu=%.c)
#
H_SOURCES    := $(notdir $(wildcard *.h))
C_SOURCES    := $(filter-out $(MAINS) $(GENERATED_C),$(notdir $(wildcard *.c)))
#
#
DEPENDS := $(MAINS:%.c=.depends/%.d)
DEPENDS += $(C_SOURCES:%.c=.depends/%.d)
DEPENDS += $(GENERATED_C:%.c=.depends/%.d)

#
#
all :: $(MAINS:%.c=%.run)

test ::

checkpoint : ; git checkpoint

$(RUNS) : $(WATER)

water_compile  : $(WATER_TREES:%.h2o=%.c)
copper_compile : $(COPPER_TREES:%.cu=%.c)

clean ::
	@rm -rf .depends .tests
	@rm -f .*~ *~ *.x test_*.run test_*.log
	@rm -f $(GENERATED_C)
	rm -f $(OBJS) $(ASMS) $(GENERATED_C:%.c=%.o) $(MAINS:%.c=%.o)

scrub :: 
	@make clean

echo ::
	@echo 'H_SOURCES    = $(H_SOURCES)'
	@echo 'C_SOURCES    = $(C_SOURCES)'
	@echo 'WATER_TREES  = $(WATER_TREES)'
	@echo 'COPPER_TREES = $(COPPER_TREES)'
	@echo 'MAINS        = $(MAINS)'
	@echo 'DEPENDS      = $(DEPENDS)'

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
.SECONDARY :: $(GENERATED_C)

##
## rules
##
%.c : %.cu $(COPPER)
	$(COPPER) --name $(@:%.c=%_ctree) --output $@ --file $<

%.c : %.h2o $(WATER)
	$(WATER) --name $(@:%.c=%_wtree) --output $@ --file $<

%.o : %.c
	$(GCC) $(CFLAGS) -c -o $@ $<

%.x : %.o ; $(GCC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

%.run : %.x
	./$< $(@:%.run=%.input) > $@ 2> $(@:%.run=%.log)
	@cat $@

#
# include dependences
#
.depends : ; @mkdir .depends
.depends/%.d : %.c .depends ; @$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<

-include $(DEPENDS)

#
# test dependences
#
.tests : ; @mkdir .tests

.tests/%.mk : test_%.c .tests ./test.rules ; @./test.rules $(@:.tests/%.mk=%) > $@

-include $(MAINS:test_%.c=./.tests/%.mk)


