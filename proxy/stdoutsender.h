#pragma once
#include "radioreader.h"

class StdoutSender {
public:
    int sendrecvLoop(RadioReader& reader);
};

