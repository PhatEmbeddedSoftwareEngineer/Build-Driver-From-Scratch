#ifndef CLOCK_H
#define CLOCK_H

#include "define.h"

namespace CLOCK_TREE_DRIVER 
{
    class ClockSource 
    {
    public:
        static ClockSource& instance();
        void clocking_HSI(void) ; 
    private:
        
        ClockSource() = default;
        ~ClockSource() = default;
        ClockSource(const ClockSource&) = delete;
        ClockSource& operator=(const ClockSource&) = delete;
    };
}

#endif