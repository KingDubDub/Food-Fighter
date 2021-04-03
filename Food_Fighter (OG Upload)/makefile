# ----------------------------
# Makefile Options
# ----------------------------

NAME ?= FOODFITE
ICON ?= icon.png
DESCRIPTION ?= "CC25: A heated RPG battle for survival!"
COMPRESSED ?= YES
ARCHIVED ?= YES

CFLAGS ?= -Wall -Wextra -Oz
CXXFLAGS ?= -Wall -Wextra -Oz

# ----------------------------

ifndef CEDEV
$(error CEDEV environment path variable is not set)
endif

include $(CEDEV)/meta/makefile.mk