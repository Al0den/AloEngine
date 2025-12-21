#include<stdlib.h>
#include<stdio.h>

#include "alo/types.hpp"

#define CASTLE1 "r1r3k1/1p1b1ppp/p1p1pn2/2P5/2B1P3/1P1P1N2/P4PPP/R3R1K1 w - - 0 1"

// 0000 0000 0000 0000 0000 0111 1111 -> From (0x7F)
// 0000 0000 0000 0011 1111 1000 0000 -> To (>> 7,  0x7F)
// 0000 0000 0011 1100 0000 0000 0000 -> Captured (>> 14, 0xF)
// 0000 0000 0100 0000 0000 0000 0000 -> EnPas (0x40000)
// 0000 0000 1000 0000 0000 0000 0000 -> Pawn Start (0x80000)
// 0000 1111 0000 0000 0000 0000 0000 -> Promotion (>> 20, 0xF)
// 0001 0000 1000 0000 0000 0000 0000 -> Castled (0x1000000)

int main() {
    AllInit();
    Uci_Loop();
    return 0;
}
