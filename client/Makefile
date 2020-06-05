TARGET  = reloc_radio-client.o
OBJFILES = radio-client.o args.o telnetserver.o message.o keepalive.o proxyreceiver.o cyclicbuffer.o
COMMON = ../common
ERRORH = $(COMMON)/error.h

all: $(TARGET)

clean:
	rm $(TARGET) $(OBJFILES) 

radio-client.o:	radio-client.cc args.h

args.o:	args.h

telnetserver.o: telnetserver.cc telnetserver.h message.h cyclicbuffer.h $(ERRORH)

message.o: message.h

cyclicbuffer.o: cyclicbuffer.cc cyclicbuffer.h

keepalive.o: keepalive.cc keepalive.h message.h $(ERRORH)

proxyreceiver.o: proxyreceiver.cc proxyreceiver.h message.h $(ERRORH)

$(TARGET): $(OBJFILES)
	ld -r $(OBJFILES) -o $(TARGET)
