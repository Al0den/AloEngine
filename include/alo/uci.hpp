#pragma once

#include "alo/types.hpp"

namespace alo {

class Uci {
public:
    void loop();
    void parseGo(char *line, SearchInfo *info, Board *pos);
    void parsePosition(const char *lineIn, Board *pos);
};

} // namespace alo

