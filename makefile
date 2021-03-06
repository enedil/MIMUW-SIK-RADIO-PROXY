CXX		   = g++
CXXVERSION = -std=c++2a
CXXWARNS   = -Wall -Wextra -Wpedantic -Wno-reorder
CXXFLAGS   = $(CXXVERSION) $(CXXWARNS) -g3 -O2 -pthread 

CLIENT = client
PROXY = proxy
COMMON = common

all: $(CLIENT) $(PROXY) $(COMMON)
	$(CXX) $(CXXFLAGS) $(COMMON)/reloc_common.o $(CLIENT)/reloc_radio-client.o -o radio-client
	$(CXX) $(CXXFLAGS) $(COMMON)/reloc_common.o $(PROXY)/reloc_radio-proxy.o -o radio-proxy

.PHONY: $(CLIENT) $(PROXY) $(COMMON) all clean


$(CLIENT):
	$(MAKE) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" -C $(CLIENT)

$(PROXY):
	$(MAKE) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" -C $(PROXY)

$(COMMON):
	$(MAKE) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" -C $(COMMON)


clean:
	rm -f radio-client radio-proxy 
	$(MAKE) -C $(CLIENT) clean
	$(MAKE) -C $(PROXY) clean
	$(MAKE) -C $(COMMON) clean
