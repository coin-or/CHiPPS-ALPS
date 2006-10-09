NFDIR := ..
ROOT  := $(PWD)
BOOSTROOT := ${HOME}/src/boost

##############################################################################

LIBTYPE      := STATIC
INSTALLDIR   := ..
SYSLD        := 
UNAME        := $(shell uname)
CXX          := g++
DEPCXX       := $(CXX) -MM
EFENCE       := -lefence
CP           := cp
LN           := ln -s
SHLINKPREFIX := -Wl,-rpath,
OPTLEVEL     := -g
OPTFLAG      := $(OPTLEVEL)
CXXFLAGS     := -I- $(OPTFLAG)

ifeq (${LIBTYPE},SHARED)
  LD         := g++ -o
  LIBLDFLAGS := -shared
  LIBEXT     := .so
  CXXFLAGS   += -fPIC
else
  LD         := ar -q
  LIBLDFLAGS :=
  LIBEXT     := .a
  CXXFLAGS   +=
endif


##############################################################################

SPACE      := $(empty) $(empty)
OPTVERSION := $(subst $(SPACE),_,$(OPTLEVEL))

##############################################################################

INCDIRS    := $(ROOT)/include $(NFDIR)/include $(BOOSTROOT)
LIBDIRS    := 
LIBS       := 
DEFINES    := 

##############################################################################
##############################################################################

LDFLAGS := $(addprefix -L,$(LIBDIRS))

##############################################################################
##############################################################################
# Paths
##############################################################################
##############################################################################

TARGETDIR = $(ROOT)/$(UNAME)$(OPTFLAG)
DEPDIR    = $(ROOT)/dep

SRCDIR    = ${ROOT} : ${ROOT}/include : ${NFDIR}/include

##############################################################################
##############################################################################
# Putting together DEF's, FLAGS
##############################################################################
##############################################################################

CXXFLAGS += $(addprefix -I,$(INCDIRS)) $(addprefix -D,$(DEFINES)) 

##############################################################################
##############################################################################
# Global source files
##############################################################################
##############################################################################

SRC  = 

SRC +=	AlpsSubTree.cpp
SRC +=	AlpsTreeNode.cpp

##############################################################################
##############################################################################
# Global rules
##############################################################################
##############################################################################

$(TARGETDIR)/%.o : %.cpp ${DEPDIR}/%.d
	@echo ""
	@echo Compiling $*.cpp
	@mkdir -p $(TARGETDIR)
	@$(CXX) $(CXXFLAGS) $(OPTFLAG) -c $< -o $@

${DEPDIR}/%.d : %.cpp
	@echo ""
	@echo Creating dependency $*.d
	@mkdir -p ${DEPDIR}
	@rm -f $*.d
	@set -e; $(DEPCXX) $(CXXFLAGS) $< \
	    | sed 's|\($(notdir $*)\)\.o[ :]*|$(TARGETDIR)/\1.o $@ : |g' \
	    > $@; [ -s $@ ] || rm -f $@

##############################################################################
##############################################################################

OBJFILES  = $(addprefix $(TARGETDIR)/, $(notdir $(SRC:.cpp=.o)))

DEPFILES  = $(addprefix $(DEPDIR)/, $(notdir $(OBJFILES:.o=.d)))

##############################################################################

.PHONY: doc default clean libalps 

default : install

install : libalps
	@echo "Installing include files..."
	@mkdir -p ${INSTALLDIR}/include
# *FIXME* : do we _really_ need all the headers?
	@${CP} ${ROOT}/include/*.h ${INSTALLDIR}/include
	@echo "Installing libraries..."
	@mkdir -p ${INSTALLDIR}/lib
	@${CP} $(TARGETDIR)/libalps$(OPTVERSION)$(LIBEXT) ${INSTALLDIR}/lib
	@rm -f ${INSTALLDIR}/lib/libalps$(LIBEXT)
	@cd ${INSTALLDIR}/lib; ${LN} libalps$(OPTVERSION)$(LIBEXT) \
        libalps$(LIBEXT)

libalps  : $(TARGETDIR)/libalps$(OPTVERSION)$(LIBEXT)

$(TARGETDIR)/libalps$(OPTVERSION)$(LIBEXT) : $(OBJFILES)
	@rm -rf Junk
	@echo ""
	@echo "Creating library $(notdir $@) ..."
	@echo ""
	@mkdir -p $(TARGETDIR)
	@rm -f $@
	$(LD) $@ $(LIBLDFLAGS) $(OBJFILES)

clean :
	rm -rf Junk
	rm -rf $(DEPDIR) 
	rm -rf $(TARGETDIR) 
	rm -rf $(TARGETDIR)

###############################################################################

%::
	@mkdir -p Junk
	touch Junk/$(notdir $@)

##############################################################################

.DELETE_ON_ERROR: ;

-include $(DEPFILES) 
