TARGET  = radio-client
OBJFILES = radio-client.o args.o telnetserver.o message.o error.o keepalive.o proxyreceiver.o

all: $(TARGET)

clean:
	rm $(TARGET) $(OBJFILES) 

radio-client.o:	radio-client.cc args.h

args.o:	args.h

telnetserver.o: telnetserver.cc telnetserver.h message.h error.h

message.o: message.h

error.o: error.h

keepalive.o: keepalive.cc keepalive.h message.h

proxyreceiver.o: proxyreceiver.cc proxyreceiver.h message.h error.h

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(OBJFILES) -o $(TARGET)
