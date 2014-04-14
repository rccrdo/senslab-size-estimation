# projetc setup
CONTIKI_PROJECT = size-estimator
all: $(CONTIKI_PROJECT)


# Setup the radio flag for the given site
ifeq ($(site),strasbourg)
	$(info Building for site 'Strasbourg' with support for 'cc1100' radio)
RADIO=WITH_CC1100
else ifeq ($(site),lille)
	$(info Building for site 'Lille' with support for 'cc2420' radio)
RADIO=WITH_CC2420
else ifeq ($(site),grenoble)
	$(error update radio flags for site 'Grenoble')
else
	${error No site given, pass site=name with name being one of 'strasbourg', 'lille' or 'grenoble'}
endif


#
# Options -fdata-sections -ffunction-sections and --gc-sections are
# to let ld strip unused code and data from the final image.
#
CFLAGS += -fdata-sections -ffunction-sections -D$(RADIO) -DPROJECT_CONF_H=\"size-estimator-conf.h\"
LDFLAGS += -Wl,--gc-sections


# Misc utils
PROJECT_SOURCEFILES += util.c radio-arb.c
PROJECTDIRS += ./

# Math
PROJECT_SOURCEFILES += distributions.c fixpoint32.c
PROJECTDIRS += math/

# Net
PROJECT_SOURCEFILES += packet-splitter.c connection-tracker.c
PROJECTDIRS += net/

# Estimators
PROJECT_SOURCEFILES += uni-size-estimator.c 
PROJECTDIRS += size-estimators/uniform

# Processes
PROJECT_SOURCEFILES += proc-epoch-syncer.c proc-size-estimator.c


# Paths to the folders containing the Contiki-OS and the wsn430 platform drivers
CONTIKI = ../../senslab-git/software/OS/Contiki/
WSN430_DRIVERS_PATH = ../../senslab-git/software/drivers/wsn430/


# Let Contiki's Makefile build us
include $(CONTIKI)/Makefile.include
