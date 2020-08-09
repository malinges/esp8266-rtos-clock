#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

MAKEFLAGS := -j$(shell nproc)

PROJECT_NAME := esp8266-rtos-clock

include $(IDF_PATH)/make/project.mk

