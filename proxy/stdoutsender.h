#pragma once
#include "radioreader.h"

// Sends music to STDOUT and metadata to STDERR.
class StdoutSender {
public:
    int sendrecvLoop(RadioReader& reader);
};

