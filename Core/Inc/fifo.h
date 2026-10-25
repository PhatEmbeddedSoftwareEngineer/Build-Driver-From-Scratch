#ifndef FIFO_H 
#define FIFO_H

#include "define.h"


namespace DriverArmCotexM4 {
    class DriverUART;
    inline void __disable_irq(void);
    inline void __enable_irq(void); 
}

class FIFO 
{
private:
    friend class DriverArmCotexM4::DriverUART;

    typedef struct
    {
        uint8_t *buf;      	    
        uint32_t buf_size;    	
        uint32_t occupy_size;   
        uint8_t *pwrite;      	
        uint8_t *pread;       	
        void (*lock)(void);	
        void (*unlock)(void);	
    }_fifo_t;

    typedef void (*lock_fun)(void);

public:
    void fifo_register(_fifo_t *pfifo, uint8_t *pfifo_buf, uint32_t size,
                            lock_fun lock, lock_fun unlock);
    void fifo_release(_fifo_t *pfifo);
    uint32_t fifo_write(_fifo_t *pfifo, const uint8_t *pbuf, uint32_t size);
    uint32_t fifo_read(_fifo_t *pfifo, uint8_t *pbuf, uint32_t size);
    uint32_t fifo_get_total_size(_fifo_t *pfifo);
    uint32_t fifo_get_free_size(_fifo_t *pfifo);
    uint32_t fifo_get_occupy_size(_fifo_t *pfifo);

    static void fifo_lock(void)
    {
        DriverArmCotexM4::__disable_irq();
    }
    static void fifo_unlock(void)
    {
        DriverArmCotexM4::__enable_irq();
    }
};

#endif // FIFO_H