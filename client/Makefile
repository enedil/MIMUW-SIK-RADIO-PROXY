TARGET  = radio-client
OBJFILES = radio-client.o

all: $(TARGET)

clean:
	rm $(TARGET) $(OBJFILES) 

radio-client.o:	radio-client.cc

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(OBJFILES) -o $(TARGET)
