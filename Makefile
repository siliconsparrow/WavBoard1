# ############################################################################
# ##
# ## Makefile for the WAVBoard firmware on Kinetis KL17.
# ##
# ##   by Adam Pierce <adam@siliconsparrow.com>
# ##   created 07-Feb-2021
# ##
# ############################################################################

TARGET = WAVBoard

# Directories in which we might find sources and dependencies.
VPATH = . \
	:platform \
	:drivers \
	:Filesystem
	
# List of all source files to compile.
SOURCES = \
	startup_MKL17Z644.S \
	system_MKL17Z644.c \
	syscalls.cpp \
	SystemIntegration.cpp \
	Gpio.cpp \
	SystemTick.cpp \
	Dma.cpp \
	Spi.cpp \
	AudioSource.cpp \
	AudioKinetisI2S.cpp \
	SDCard.cpp \
	diskio.cpp \
	ff.c \
	Filesystem.cpp \
	main.cpp

# Defines.
DEFINES = -DCPU_MKL17Z32VLH4

# Include paths
INCLUDES = -I. \
	-Iplatform \
	-Iplatform/CMSIS \
	-IFilesystem \
	-Idrivers \

# Output directories
DEBUGPATH = Debug
RELEASEPATH = Release

# Linker script
LINKER_SCRIPT_RELEASE = platform/MKL17Z32xxx4_flash.ld
LINKER_SCRIPT_DEBUG = platform/MKL17Z32xxx4_flash.ld

# Additional libraries.
LIBS =

# Compiler flags for both C and C++.
COMMONFLAGS = -fmessage-length=0 -mthumb -mcpu=cortex-m0 -specs=nano.specs -fshort-wchar -fomit-frame-pointer -ffunction-sections -fdata-sections
 
# Debug and optimization flags.
DEBUGFLAGS = -g -O0 #-D__DEBUG 
RELEASEFLAGS = -O3 -g0

# Additional compiler flags for C
CCONLYFLAGS = -std=gnu99

# Additional compiler flags for C++.
CXXONLYFLAGS = -fno-exceptions -fno-rtti

# Tools we are using. Make sure the system path allows access to all of these.
BUILDPREFIX = arm-none-eabi-
AS      = $(BUILDPREFIX)gcc
CC      = $(BUILDPREFIX)gcc
CXX     = $(BUILDPREFIX)g++
LD      = $(BUILDPREFIX)g++
OBJCOPY = $(BUILDPREFIX)objcopy
SIZE    = $(BUILDPREFIX)size
MKDIR   = mkdir
RMDIR   = rm -Rf

# Build rules.
CFLAGS = $(COMMONFLAGS) $(CCONLYFLAGS) $(DEFINES) $(INCLUDES)
CXXFLAGS = $(COMMONFLAGS) $(CXXONLYFLAGS) $(DEFINES) $(INCLUDES)
ASFLAGS = $(COMMONFLAGS)
LDFLAGS_DEBUG = $(COMMONFLAGS) -Wl,--no-wchar-size-warning -Wl,--gc-sections -Xlinker -T$(LINKER_SCRIPT_DEBUG)
LDFLAGS_RELEASE = $(COMMONFLAGS) -Wl,--no-wchar-size-warning -Wl,--gc-sections -Xlinker -T$(LINKER_SCRIPT_RELEASE) 
OBJS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.S,%.o,$(notdir $(SOURCES)))))


# ###################################
# Build targets

release: $(RELEASEPATH) $(RELEASEPATH)/$(TARGET).hex

debug: $(DEBUGPATH) $(DEBUGPATH)/$(TARGET).hex

# Delete working files and objects for both debug and release.
clean:
	$(RM)    $(RELEASEPATH)/$(TARGET).hex
	$(RM)    $(RELEASEPATH)/$(TARGET).elf
	$(RM)    $(RELEASEPATH)/$(TARGET).map
	$(RM)    $(addprefix $(RELEASEPATH)/,$(OBJS))
	$(RMDIR) $(RELEASEPATH)
	$(RM)    $(DEBUGPATH)/$(TARGET).hex
	$(RM)    $(DEBUGPATH)/$(TARGET).elf
	$(RM)    $(DEBUGPATH)/$(TARGET).map
	$(RM)    $(addprefix $(DEBUGPATH)/,$(OBJS))
	$(RMDIR) $(DEBUGPATH)


# ###################################
# RELEASE CONFIGURATION

# Create directory for object and binary files.
$(RELEASEPATH):
	$(MKDIR) $(RELEASEPATH)

$(RELEASEPATH)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c -o $(RELEASEPATH)/$(@F) $<

$(RELEASEPATH)/%.o: %.c
	$(CC) $(CFLAGS) $(RELEASEFLAGS) -c -o $(RELEASEPATH)/$(@F) $<
	
$(RELEASEPATH)/%.o: %.s
	$(AS) $(ASFLAGS) -c -o $(RELEASEPATH)/$(@F) $<

# Linker.
$(RELEASEPATH)/$(TARGET).elf: $(addprefix $(RELEASEPATH)/,$(OBJS)) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS_RELEASE) -o $(RELEASEPATH)/$(TARGET).elf $(addprefix $(RELEASEPATH)/,$(OBJS)) $(LIBS) -Xlinker -Map=$(RELEASEPATH)/$(TARGET).map
	@# Print the size of the binary image.
	$(SIZE) $(RELEASEPATH)/$(TARGET).elf

# Create a HEX file for firmware flashing.
$(RELEASEPATH)/$(TARGET).hex: $(RELEASEPATH)/$(TARGET).elf
	$(OBJCOPY) -O ihex $(RELEASEPATH)/$(TARGET).elf $(RELEASEPATH)/$(TARGET).hex


# ###################################
# DEBUG CONFIGURATION

$(DEBUGPATH)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -c -o $(DEBUGPATH)/$(@F) $<

$(DEBUGPATH)/%.o: %.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c -o $(DEBUGPATH)/$(@F) $<
	
$(DEBUGPATH)/%.o: %.S
	$(AS) $(ASFLAGS) -c -o $(DEBUGPATH)/$(@F) $<

# Linker.
$(DEBUGPATH)/$(TARGET).elf: $(addprefix $(DEBUGPATH)/,$(OBJS)) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS_DEBUG) -o $(DEBUGPATH)/$(TARGET).elf $(addprefix $(DEBUGPATH)/,$(OBJS)) $(LIBS) -Xlinker -Map=$(DEBUGPATH)/$(TARGET).map
	@# Print the size of the binary image.
	$(SIZE) $(DEBUGPATH)/$(TARGET).elf

# Create a HEX file for firmware flashing.
$(DEBUGPATH)/$(TARGET).hex: $(DEBUGPATH)/$(TARGET).elf
	$(OBJCOPY) -O ihex $(DEBUGPATH)/$(TARGET).elf $(DEBUGPATH)/$(TARGET).hex

# Create directory for object and binary files.
$(DEBUGPATH):
	$(MKDIR) $(DEBUGPATH)
