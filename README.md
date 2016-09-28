# memory_manager

![Alt text](https://github.com/maghniem/memory_manager/blob/master/tlb.png "TLB Lookup Figure")

Objectives:

Read a text file containing 32-bit integer values representing logical addresses.
Mask the left most 16 bits and use the right most bits for an 8-bit offset and an 8-bit page
number.
Translate the logical addresses to physical addresses using a TLB and page table
Handle Page Faults and TLB-miss events by implementing Demand Paging

Implementation:

The TLB itself will be represented using an array of 16 positions holding structures of TLB
entries. The TLB entries will comprise of a logical address and a physical address. The TLB will be
checked after the page number is extracted from the logical address and adding the specified mapping
to the TLB by replacing the oldest one (in FIFO fashion)

The physical memory will be represented with an array of 65,536 bytes using the data type
int8_t from the header <stdint.h>. The page table will also be represented using an array of integers
holding 256 values.
Our program will need to treat BACKING_STORE.bin as a random-access file so that it can
randomly seek to certain positions of the file for reading. The function used was the memory map

```C
function mmap():
  mapped_backing =
  mmap(
  0 ,
  MEMORY_SIZE ,
  PROT_READ, MAP_PRIVATE ,
  backing_store_descriptor ,
  0
);
```

Using this function it is possible to map a file to a region of memory, allowing you to access the file
just like an array in the program. It is more efficient than read and write system calls as only parts of
the file the program is accessing are loaded.
Because we are only concerned with the lower 16 bits of the given 32-bit address values,
masking the correct bits was required when calculating the offset and logical address values. This was
done with calculations shown below:
```C
int offset = logical_address & OFFSET_MASK;
int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;
```
The offset mask was 0xFF (255) which turns the upper 24 bits to 0 when anded to the 32-bit number
read into logical_address. To obtain the logical page value the 32-bit number was first shifted to the
left 8 bits, then anded with 0xFF again.
After extracting the logical page number form the 32-bit address using above methods, the TLB
will be consulted to return the corresponding physical address using the function below:
```C
int TLB_lookup(uint8_t logical_address_page){
  unsigned int i;
  for (i = MAX((TLB_index-TLB_SIZE),0); i < TLB_index; i++){
    struct TLB_Entry_* entry = &( TLB[i % TLB_SIZE] );
    if (entry->logical_address == logical_address_page){
      return entry->physical_address;
    }
  }
  return -1;
}
```

The function first determines where to start looking in the TLB and iterates through the array trying to
match the logical address given to a logical address in the TLB. If a match is found, it will return the
physical address saved as part of the entry structure or -1 otherwise. A hit will ultimatly give the frame
number. If a miss is encountered, the page table is consulted:
```C
physical_page = page_table[logical_page];
```
if this also returns a -1, indicated an empty page, a fault has occurred. Otherwise this address is added
into the TLB using this function:
```C
void TLB_addition(uint8_t logical_address, uint8_t physical_address){
  struct TLB_Entry_* entry = &( TLB[TLB_index % TLB_SIZE] );
  TLB_index++;
  entry->logical_address = logical_address;
  entry->physical_address = physical_address;
  return;
}
```
A new entry is created at the correct location, replacing the oldest entries as the index circulates
through the beginning to the end of the array.
To handle Page Faults, a 256-byte page is read from the BACKING_STORE.bin file that was
previously mentioned to be memory mapped and stored in an available page frame in physical
memory:
```C
memcpy(
  memory + physical_page * PAGE_SIZE ,
  mapped_backing + logical_page * PAGE_SIZE ,
  PAGE_SIZE
);

page_table[logical_page] = physical_page;
```
Pointer arithmetic is done to retrieve the correct positions in memory and the mapped_backing store file
in memory and copy the values.
At the end of these calculations and address retrievals, the value of the signed byte at the
physical address will be outputted along with the Page-Fault rate and TLB hit rate:
```C
int physical_address = (physical_page << OFFSET_BITS) | offset;
int8_t value = memory[physical_page * PAGE_SIZE + offset];
```

Running of Program and Statistics:

The program will compiled with the follow command:
```BASH
gcc memory_manager.c -o myProject
```
The program was run with the command:
```BASH
./myProject addresses.txt > output.txt
```

The complete output is redirected into a text file. The first
10 lines are as follows:
```BASH
$ head -10 output.txt
Virtual address: 16916 Physical address: 20 Value: 0
Virtual address: 62493 Physical address: 285 Value: 0
Virtual address: 30198 Physical address: 758 Value: 29
Virtual address: 53683 Physical address: 947 Value: 108
Virtual address: 40185 Physical address: 1273 Value: 0
Virtual address: 28781 Physical address: 1389 Value: 0
Virtual address: 24462 Physical address: 1678 Value: 23
Virtual address: 48399 Physical address: 1807 Value: 67
Virtual address: 64815 Physical address: 2095 Value: 75
Virtual address: 18295 Physical address: 2423 Value: -35
$
```
