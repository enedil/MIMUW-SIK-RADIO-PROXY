TARGET  = reloc_common.o
OBJFILES = error.o socket.o

all: $(TARGET)

clean:
	rm $(TARGET) $(OBJFILES) 

error.o: error.cc error.h

socket.o: socket.cc socket.h

$(TARGET): $(OBJFILES)
	ld -r $(OBJFILES) -o $(TARGET)
