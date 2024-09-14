# Makefile for building the project on a linux computer to either a
# linux binary or a windows binary.
# For the windows binary, the WBF definition defines the locations
# where the GLFW and Vulkan libraries and headers must be stored.

# Commonly changed definitions
CFLAGS=-lvulkan
LFLAGS=-lglfw
WFLAGS= -lgdi32 -l:glfw.a


# Uncommonly changed definitions
APP=tiles

SOURCE=src

OUTPUT=intermediate
DEBUGOUTPUT=debug
LINOUTPUT=lin
WINOUTPUT=win

LCC=cc
LAF=-lglfw -lvulkan 

WCC=x86_64-w64-mingw32-gcc-win32
WBF=-L/usr/share/winlibs/ -I/usr/share/winincludes/
WAF=-l:glfw.a -lgdi32 -lssp -lvulkan

# Generative definitions
CFILES=$(wildcard $(SOURCE)/*.c)
OFILES=$(CFILES:$(SOURCE)/%.c=%.o)

WINTARGET=$(OUTPUT)/$(WINOUTPUT)
LINTARGET=$(OUTPUT)/$(LINOUTPUT)
LINDEBUGTARGET=$(OUTPUT)/$(DEBUGOUTPUT)/$(LINOUTPUT)


# Processes
.PHONY: test linux windows clean

test: $(OFILES:%=$(LINDEBUGTARGET)/%)
	$(LCC) $^ -o $(APP) $(LAF)
	./$(APP)

linux: $(OFILES:%=$(LINTARGET)/%)
	$(LCC) $^ -o $(APP) $(LAF)

windows: $(OFILES:%=$(WINTARGET)/%)
	$(WCC) $(WBF) $^ -o $(APP).exe $(WAF)

clean:
	rm -rf $(OUTPUT) $(APP) $(APP).exe


# File generation
$(LINTARGET)/%.o: $(SOURCE)/%.c
	mkdir -p $(LINTARGET)
	$(LCC) $< -c -o $@

$(LINDEBUGTARGET)/%.o: $(SOURCE)/%.c
	mkdir -p $(LINDEBUGTARGET)
	$(LCC) -DDEBUG $< -c -o $@

$(WINTARGET)/%.o: $(SOURCE)/%.c
	mkdir -p $(WINTARGET)
	$(WCC) $(WBF) $< -c -o $@
