#CXX		 = clang++
CXX		 = g++
#CXXFLAGS = -Weverything -std=c++2a -Wno-padded -Wno-reorder -Wno-c++98-compat -Wno-shadow-field-in-constructor -g3 -O2 -pthread
CXXFLAGS = -std=c++2a -Wall -Wextra -Wno-reorder -g3 -O2 -pthread 
#CXXFLAGS += -fsanitize=undefined,address

#CXXFLAGS = -std=c++17
DBG 	 = -g3

CLIENT = client
PROXY = proxy

all: $(CLIENT) $(PROXY)
	install $(CLIENT)/radio-client .
	install $(PROXY)/radio-proxy .

.PHONY: $(CLIENT) $(PROXY) all clean


$(CLIENT):
	$(MAKE) CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" -C $(CLIENT)

$(PROXY):
	$(MAKE) CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" -C $(PROXY)

clean:
	rm radio-client
	rm radio-proxy
	$(MAKE) -C $(CLIENT) clean
	$(MAKE) -C $(PROXY) clean

