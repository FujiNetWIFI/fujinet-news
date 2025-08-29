AS      = wasm -q
ASFLAGS = -0 -mt -bt=DOS
CC      = wcc -q
CFLAGS  = -0 -bt=dos -ms -I../fujicom -osh $(CPPFLAGS)
LD	= wlink OPTION quiet
LDFLAGS = SYSTEM dos OPTION MAP
DIST_DIR = dist
TARGET = $(DIST_DIR)/news.exe

all: $(TARGET) $(DIST_DIR)

$(TARGET): news.obj screen.obj globals.obj topics.obj select.obj articles.obj article.obj fetch.obj

.obj.exe:
	$(LD) $(LDFLAGS) NAME $@ file {$<} LIBRARY {clibs.lib}
.c.obj: .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ $<
.asm.obj: .AUTODEPEND
	$(AS) $(ASFLAGS) -fo=$@ $<

disk: $(TARGET)

$(DIST_DIR):
	mkdir -p $@

clean : .SYMBOLIC
	rm -f $(TARGET) *.obj *.map *.err
