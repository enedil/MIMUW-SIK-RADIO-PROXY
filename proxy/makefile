TARGET  = reloc_radio-proxy.o
OBJFILES = radio-proxy.o args.o radioreader.o radiosender.o stdoutsender.o
COMMON = ../common
ERRORH = $(COMMON)/error.h

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJFILES) 

radio-proxy.o:	radio-proxy.cc args.h radioreader.h radiosender.h $(ERRORH)

radioreader.o:	radioreader.cc radioreader.h $(ERRORH)

radiosender.o: radiosender.cc radiosender.h radioreader.h $(ERRORH)

stdoutsender.o: stdoutsender.cc stdoutsender.h radioreader.h $(ERRORH)

args.o:	args.cc args.h

$(TARGET): $(OBJFILES)
	ld -r $(OBJFILES) -o $(TARGET)
