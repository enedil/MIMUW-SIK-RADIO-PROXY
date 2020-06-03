TARGET  = radio-proxy
OBJFILES = radio-proxy.o args.o radioreader.o radiosender.o

all: $(TARGET)

clean:
	rm $(TARGET) $(OBJFILES) 

radio-proxy.o:	radio-proxy.cc args.h radioreader.h radiosender.h

radioreader.o:	radioreader.cc radioreader.h

radiosender.o: radiosender.cc radiosender.h

args.o:	args.cc args.h

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(OBJFILES) -o $(TARGET)
