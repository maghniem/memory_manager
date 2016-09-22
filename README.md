# memory_manager
Objectives:
Read a text file containing 32-bit integer vales representing logical addresses.
Mask the left most 16 bits and use the right most bits for an 8-bit offset and an 8-bit page
number.
Translate the logical addresses to physical addresses using a TLB and page table
Handle Page Faults and TLB-miss events by implementing Demand Paging
