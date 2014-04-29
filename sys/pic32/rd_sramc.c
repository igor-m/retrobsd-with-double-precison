/*
 * Driver for external SRAM-CPLD based Swap and Filesystem devices
 *
 * This version is for 8MB RAMDISK v.1.1 and compatible
 * Pito 7.4.2014 - PIC32MX PMP bus version
 * Pito 28.4.2014 - Mod for 2 Units
 * Under by retrobsd.org used Licence
 * No warranties of any kind
 *
 * Interface:
 *  PMD<7:0> - connected to PMP data bus
 *  PMRD      - fetch a byte from memory to data<7:0>, increment address, PMRD
 *  PMWR      - write a byte data[7:0] to memory, increment address, PMWR
 *  PMA0      - HIGH - write Address from data<3:0> in 6 steps: high nibble ... low nibble
 *            - LOW - write/read Data
 *            
 * Signals PMRD, PMWR are active LOW and idle HIGH
 * Signal PMA0 is LOW when accessing RAM Data, and HIGH when accessing RAM Addresses
 * 
 *
 */
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "rdisk.h"

#include "debug.h"

int sw_dkn = -1;                /* Statistics slot number */

// 8MB Ramdisk v.1.1. wiring
// PMP         RAMDISK
// ===================
// PMD<D0-D7>  D0-D7
// PMRD        /RD
// PMWR        /WR
// PMA<0>      /DATA
// PMA<1>      0-Unit0
// PMA<10>     0-Unit1

//#define NDATA (1<<0)
//#define UNIT0 (1<<1)
//#define UNIT1 (1<<10)

// RD and WR pulses duration settings
// Minimal recommended settings, increase them when unstable
// No warranties of any kind
// for 120MHz clock, 70ns PSRAM, 8MB Ramdisk v.1.1.
#define ADR_PULSE  1
#define WR_PULSE   5
#define RD_PULSE   11

// for 80MHz clock, 70ns PSRAM, 8MB Ramdisk v.1.1.
//#define ADR_PULSE 1
//#define WR_PULSE 3
//#define RD_PULSE 8

// for 120MHz clock, 55ns SRAM
//#define ADR_PULSE 1
//#define WR_PULSE 3
//#define RD_PULSE 6

// for 80MHz clock, 55ns SRAM
//#define ADR_PULSE 1
//#define WR_PULSE 2
//#define RD_PULSE 4

typedef union {
	unsigned int value;
	struct {
        unsigned nib1: 4;  // lowest nibble
        unsigned nib2: 4;
        unsigned nib3: 4;
        unsigned nib4: 4;
        unsigned nib5: 4;
        unsigned nib6: 4;
        unsigned nib7: 4;
        unsigned nib8: 4;  // highest nibble
	};
} nybbles;


/*
 * Load the 24 bit address to Ramdisk.
 * 
 */
inline static void dev_load_address (unsigned int addr)
{
	    nybbles temp;
	    temp.value = addr;

        while(PMMODE & 0x8000);  // Poll - if busy, wait

        PMMODE = 0b10<<8 | (ADR_PULSE<<2);  // full ADR speed

        PMDIN = temp.nib6;           /* write 4 bits */
 
        while(PMMODE & 0x8000);  // Poll - if busy, wait
        PMDIN = temp.nib5;           /* write 4 bits */
 
        while(PMMODE & 0x8000);  // Poll - if busy, wait
        PMDIN = temp.nib4;           /* write 4 bits */
 
        while(PMMODE & 0x8000);  // Poll - if busy, wait
        PMDIN = temp.nib3;           /* write 4 bits */

        while(PMMODE & 0x8000);  // Poll - if busy, wait
        PMDIN = temp.nib2;           /* write 4 bits */

        while(PMMODE & 0x8000);  // Poll - if busy, wait
        PMDIN = temp.nib1;           /* write 4 bits */
}

/*
 * Get number of kBytes on the disk.
 * Return nonzero if successful.
 */
int sramc_size ( int unit )
{
    int srsize;

    switch (unit) {
        case 0: srsize = 8192; break;
        case 1: srsize = 8192; break;
        }
	return srsize;
}

/*
 * Read a block of data.
 */
inline int sramc_read (int unit, unsigned int blockno, register char *data, register unsigned int nbytes)
{

//printf("sramc%d: rd blockno %u, nbytes %u, addr %p\n", unit, blockno, nbytes, data);

    while(PMMODE & 0x8000); // Poll - if busy, wait

    switch (unit) {
        // set Unit address and ADDRESS mode (1)
        case 0: PMADDR = 0b10000000001;  break;
        case 1: PMADDR = 0b00000000011;  break;
        } 

	dev_load_address (blockno * DEV_BSIZE);

    while(PMMODE & 0x8000); // Poll - if busy, wait

    PMMODE = 0b10<<8 | (RD_PULSE<<2);  // read slowly

    PMADDR = PMADDR & 0b10000000010; // set DATA mode
    
    PMDIN; // Read the PMDIN to clear previous data and latch new data
        
    while (nbytes--) {
                    while(PMMODE & 0x8000); // Poll - if busy, wait before reading
                    *data++ = PMDIN;   /* read a byte of data */
                    }   
    
    while(PMMODE & 0x8000); // Poll - if busy, wait    
    PMADDR = 0b10000000011; // deselect             
	return 1;
}

/*
 * Write a block of data.
 */
inline int sramc_write (int unit, unsigned int blockno, register char *data, register unsigned int nbytes)
{

//printf("sramc%d: wr blockno %u, nbytes %u , addr %p\n", unit, blockno, nbytes, data);

    while(PMMODE & 0x8000); // Poll - if busy, wait

    switch (unit) {
        // set Unit address and ADDRESS mode (1)
        case 0: PMADDR = 0b10000000001;  break;
        case 1: PMADDR = 0b00000000011;  break;
        }

    dev_load_address (blockno * DEV_BSIZE);

    while (PMMODE & 0x8000); // Poll - if busy, wait

    PMMODE = 0b10<<8 | (WR_PULSE<<2);  // faster with write

    PMADDR = PMADDR & 0b10000000010; // set DATA mode

        while(nbytes--) {
                    while(PMMODE & 0x8000);  // Poll - if busy, wait
                    PMDIN = (*data++);   /* write a byte of data*/ 
                    }

    while(PMMODE & 0x8000); // Poll - if busy, wait
    PMADDR = 0b10000000011; // deselect
	return 1;
}


/*
 * Init the disk.
 */
void sramc_init (int unit)
{

 //   printf("ramdisk%d: init\n",unit);
	struct buf *bp;
	
    // Initialize PMP hardware

    PMCON = 0;   // disable PMP
    asm volatile ("nop"); // Errata

	PMCON = 1<<9 | 1<<8;   // Enable RD and WR

    //         MODE      WAITB     WAITM    WAITE
    PMMODE =  0b10<<8  |    0   | (14<<2) | 0    ; // Mode2 Master 8bit

    PMAEN = 0b10000000011;  // PMA<>, use A10,A1,A0

    PMADDR = 0b10000000010;  // start with DATA mode

    PMCONSET = 1<<15;   // PMP enabled       
    asm volatile ("nop"); 

    // make a couple of dummy reads - it refreshes the cpld internals a little bit :)
    switch (unit) {
    
    case 0: PMADDR = 0b10000000000;  // start with DATA mode
            while(PMMODE & 0x8000); // Poll - if busy, wait before reading
            PMDIN;   /* read a byte of data */
            while(PMMODE & 0x8000); // Poll - if busy, wait before reading
            PMDIN;   /* read a byte of data */
        
            bp = prepartition_device("sramc0");

            if(bp) {
                sramc_write(0, 0, bp->b_addr, 512);
                brelse(bp);
            }
           // printf("sramc%d: init done\n",unit);
            break;


    case 1: PMADDR = 0b00000000010;  // start with DATA mode
            while(PMMODE & 0x8000); // Poll - if busy, wait before reading
            PMDIN;   /* read a byte of data */
            while(PMMODE & 0x8000); // Poll - if busy, wait before reading
            PMDIN;   /* read a byte of data */
        
            bp = prepartition_device("sramc1");

            if(bp) {
                sramc_write(1, 0, bp->b_addr, 512);
                brelse(bp);
            }
           // printf("sramc%d: init done\n",unit);
            break;
    }

    while(PMMODE & 0x8000); // Poll - if busy, wait
    PMADDR = 0b10000000011;  // go with with ADDRESS mode
}

/*
 * Open the disk.
 */
int sramc_open (int unit)
{
    // printf("sramc%d: open done\n",unit);
	return 0;
}
