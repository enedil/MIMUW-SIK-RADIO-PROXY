TARGET  = radio-proxy
OBJFILES = radio-proxy.o args.o radioreader.o radiosender.o stdoutsender.o error.o

all: $(TARGET)

clean:
	rm $(TARGET) $(OBJFILES) 

radio-proxy.o:	radio-proxy.cc args.h radioreader.h radiosender.h error.h

radioreader.o:	radioreader.cc radioreader.h

radiosender.o: radiosender.cc radiosender.h radioreader.h

stdoutsender.o: stdoutsender.cc stdoutsender.h radioreader.h

args.o:	args.cc args.h

error.o: error.cc error.h

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(OBJFILES) -o $(TARGET)
