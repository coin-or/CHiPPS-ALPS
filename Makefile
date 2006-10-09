# Look at and if necessary edit the following files:
# - ../Makefiles/Makefile.location
# - Makefile.Alps
###############################################################################

##############################################################################
##############################################################################
# You should not need to edit below this line.
##############################################################################
##############################################################################

export CoinDir := $(shell cd ..; pwd)
export MakefileDir := $(CoinDir)/Makefiles
export AlpsDir := $(CoinDir)/Alps

include ${MakefileDir}/Makefile.coin
include ${MakefileDir}/Makefile.location
include ${AlpsDir}/Makefile.config

###############################################################################

.DELETE_ON_ERROR:

.PHONY: default install clean library unitTest libdepend libAlps doc

default: install
libAlps: library

libAlps: library

install library: libdepend
	${MAKE} -f Makefile.Alps CXX=${ALPSCXX} $@

libdepend:
	(cd $(CoinDir)/Coin && $(MAKE) install)

unitTest: 
	(cd Test && ${MAKE} unitTest)

clean: 
	@rm -rf Junk
	@rm -rf $(UNAME)*
	@rm -rf dep
	@rm -f $(CoinDir)/lib/libAlps*

doc:
	doxygen $(MakefileDir)/doxygen.conf
