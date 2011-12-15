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
PREFIX  = /tools/Water
BINDIR  = $(PREFIX)/bin
INCDIR  = $(PREFIX)/include
LIBDIR  = $(PREFIX)/lib

WATER      := water.vm
WATER.test := water.vm
WATER.ext  := hg

TIME      := $(shell date +T=%s.%N)
TERM      := dump
COLORTERM := dump
HERE      := $(shell pwd)
DEPENDS   :=

PATH := $(HERE):$(PATH)

##
COPPER     := /tools/Copper/bin/copper
COPPER_LIB := -L/tools/Copper/lib -lCopper
COPPER_INC := -I/tools/Copper/include
##
MERCURY     := /tools/Mercury/bin/mercury
MERCURY_INC := -I/tools/Mercury/include
MERCURY_LIB := -L/tools/Mercury/lib -lMercury

#
#
#
GCC      := gcc
DBFLAGS  := -ggdb -Wall -mtune=i686
INCFLAGS := $(COPPER_INC) $(COPPER_INC)
CFLAGS   := $(DBFLAGS) $(INCFLAG) 
LIBFLAGS := $(COPPER_LIB) $(MERCURY_LIB)
#
AR       := ar
ARFLAGS  := rcu
#
RANLIB := ranlib

#
#
MAINS     := water_main.c
LANGS     := water.cu
C_SOURCES := $(filter-out $(MAINS) $(LANGS:%.cu=%.c) compiler.c,$(notdir $(wildcard *.c)))
H_SOURCES := $(filter-out compiler.h, $(notdir $(wildcard *.h)))

ASMS    := $(C_SOURCES:%.c=%.s)
OBJS    := $(C_SOURCES:%.c=%.o)
TSTS    := $(notdir $(wildcard *.h2o))
RUNS    := $(TSTS:test_%.h2o=test_%.run)
DEPENDS := $(C_SOURCES:%.c=.depends/%.d) $(MAINS:%.c=.depends/%.d) $(LANGS:%.cu=.depends/%.d)

all :: $(WATER)

test :: $(RUNS)
	@make --no-print-directory -C tests all
	@echo all test runs

install :: $(BINDIR)/water
install :: $(H_SOURCES:%=$(INCDIR)/%)
install :: $(LIBDIR)/libWater.a

checkpoint : ; git checkpoint

$(RUNS) : water.vm

clean ::
	@make --no-print-directory -C tests $@
	@rm -rf .depends water_ver.h
	@rm -f .*~ *~ 
	@rm -f $(TSTS:test_%.h2o=test_%.tree)
	@rm -f $(TSTS:test_%.h2o=test_%.run)
	rm -f $(OBJS) $(ASMS) $(MAINS:%.c=%.o) $(LANGS:%.cu=%.o) compiler.o water.vm libWater.a

scrub :: 
	@make clean
	@make --no-print-directory -C tests $@
	rm -f $(LANGS:%.cu=%.c)

water.vm : water_main.o $(LANGS:%.cu=%.o)  compiler.o libWater.a 
	$(GCC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

water.o : water.c  ; $(GCC) $(DBFLAGS) $(INCFLAGS) -c -o $@ $<
water.c : water.cu

libWater.a : $(H_SOURCES)
libWater.a : $(C_SOURCES:%.c=%.o)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $(C_SOURCES:%.c=%.o)
	$(RANLIB) $@

$(BINDIR) : ; [ -d $@ ] || mkdir -p $@
$(INCDIR) : ; [ -d $@ ] || mkdir -p $@
$(LIBDIR) : ; [ -d $@ ] || mkdir -p $@

$(BINDIR)/water : $(BINDIR) $(WATER)
	cp -p $(WATER) $(BINDIR)/water
	strip $(BINDIR)/water

$(H_SOURCES:%=$(INCDIR)/%) : $(INCDIR) $(H_SOURCES)
	cp -p $(H_SOURCES) $(INCDIR)/

$(LIBDIR)/libWater.a : $(LIBDIR) libWater.a
	cp -p libWater.a $(LIBDIR)/libWater.a

water_ver.h : FORCE
	@./Version.gen WATER_VERSION water_ver.h

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
.PHONY :: FORCE

##
## rules
##

%.o : %.c
	@echo $(GCC) $(DBFLAGS) -c -o $@ $<
	@$(GCC) $(CFLAGS) -c -o $@ $<

%.c : %.cu $(COPPER)
	$(COPPER) --name $(@:%.c=%_graph) --output $@ --file $<

test_%.tree : test_%.h2o $(WATER.test)
	./$(WATER.test) --name $(@:%.tree=%_graph) --output $@ --file $<

test_%.run : test_%.tree
	$(GCC) -x c $(CFLAGS) -I. -S -o $@ $<
	@mv $< $@

.depends : ; @mkdir .depends

.depends/%.d : %.c .depends ; @$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<

-include $(DEPENDS)


