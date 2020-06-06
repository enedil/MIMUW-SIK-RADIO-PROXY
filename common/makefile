TARGET  = reloc_common.o
OBJFILES = error.o socket.o message.o

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJFILES) 

error.o: error.cc error.h

socket.o: socket.cc socket.h

message.o: message.cc message.h

$(TARGET): $(OBJFILES)
	ld -r $(OBJFILES) -o $(TARGET)
