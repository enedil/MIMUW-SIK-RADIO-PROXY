TARGET  = reloc_radio-client.o
OBJFILES = radio-client.o args.o telnetserver.o keepalive.o proxyreceiver.o cyclicbuffer.o
COMMON = ../common
ERRORH = $(COMMON)/error.h
SOCKETH = $(COMMON)/socket.h
ADDRESSH = $(COMMON)/address.h
MESSAGEH = $(COMMON)/message.h

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJFILES) 

radio-client.o:	radio-client.cc args.h

args.o:	args.h

telnetserver.o: telnetserver.cc telnetserver.h message.h cyclicbuffer.h $(ERRORH) $(SOCKETH) $(ADDRESSH) $(MESSAGEH)

cyclicbuffer.o: cyclicbuffer.cc cyclicbuffer.h

keepalive.o: keepalive.cc keepalive.h message.h $(ERRORH) $(MESSAGEH)

proxyreceiver.o: proxyreceiver.cc proxyreceiver.h message.h $(ERRORH) $(MESSAGEH)

$(TARGET): $(OBJFILES)
	ld -r $(OBJFILES) -o $(TARGET)
