#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


//max macro defined for portability
//will find the maximum between two
//integer values
#define MAX(a,b)\
({ __typeof__ (a) _a = (a);\
  __typeof__ (b) _b = (b);\
  _a > _b ? _a : _b; })

#define TLB_SIZE 0x10		//16 entries in the TLB
#define PAGES 0x100		 	//256 number of pages
#define PAGE_MASK 0xFF	//255 mask value for mask operation
#define PAGE_SIZE 0x100	//256
#define OFFSET_BITS 0x8	//8 offset bit size
#define OFFSET_MASK 0xFF	//255
#define MEMORY_SIZE (PAGES * PAGE_SIZE) //Physical memory of 65,536 bytes


//entries will be handled as a structure
//of corresponding logical and physical addresses
struct TLB_Entry_{
	uint8_t logical_address;
	uint8_t physical_address;
};

//the TLB will be held in an array of 16 entries total
struct TLB_Entry_ TLB[TLB_SIZE];

//keep track of which entry and total entries
int TLB_index = 0;

//initilize the empty table of logical pages with -1 values, -1 acts as page not in table yet
int page_table[PAGES] = {[0 ... PAGES - 1] = -1};
//memory array of 65,536 bytes
int8_t memory[MEMORY_SIZE];
//pointer to backing_store mapped file
int8_t* mapped_backing;

//prototypes
int TLB_lookup(uint8_t logical_address_page);
void TLB_addition(uint8_t logical_address, uint8_t physical_address);

//start of program
int main(int argc, char** argv){
  //error messages
	const char *input_error =
		"\nProper program usage is\
		./PROGRAM_NAME addresses.txt\n";

	const char *open_err_1 =
		"\nError opening file 'BACKING_STORE.bin'\n";

  const char *open_err_2 =
    "\nError Opening File '%s': (%s)\n";

  const char *map_err =
    "\nError mapping 'BACKING_STORE.bin'\n";


	//check parameters
	if (argc != 2){
		fprintf(stderr, "%s", input_error);

		exit (EXIT_FAILURE);
	}
  //statistics constants
	int total = 0;
	int hits = 0;
	int faults = 0;

  //open and create a file descriptor of the backing_store
	int backing_store_descriptor;
	backing_store_descriptor = open("BACKING_STORE.bin", O_RDONLY);
	if (backing_store_descriptor < 0){
		fprintf(stderr, "%s", open_err_1);
    close(backing_store_descriptor);
		exit (EXIT_FAILURE);
	}

  //open addresses.txt and check for error
  FILE *address_list = fopen(argv[1], "r");
  if (address_list == NULL){
    fprintf (stderr, open_err_2, argv[1], strerror(errno) );
    close(backing_store_descriptor);
    exit (EXIT_FAILURE);
  }

  // BACKING_STORE.bin must be a random-access file so program can randomly
  //seek to certain positions of the file for reading.
  //therefore it will be mapped to a ram location and accessed
  //like an array with mmap();
	mapped_backing =
		mmap(

			0 ,
			MEMORY_SIZE ,
			PROT_READ, MAP_PRIVATE ,
			backing_store_descriptor ,
			0
		);
    //check for map failure
    if (mapped_backing == ((int8_t *)MAP_FAILED)){
      fprintf(stderr, "%s", map_err);
      close(backing_store_descriptor);
      fclose(address_list);
      munmap(mapped_backing, MEMORY_SIZE);
      exit (EXIT_FAILURE);
    }

  //variable to hold each address read from file
	 int logical_address;
  //position/number of the next free page in memeory
	 uint8_t unallocated_page = 0;

  //main loop to load each addresses
	while( fscanf(address_list, "%d", &logical_address) != EOF ){
    //update number of addresses read
		total++;
			//get the lowest 8 bits by masking the upper 16
      //as well as the next 8 bits i.e masking the upper 24 bits
      //this will be the offset value
		int offset = logical_address & OFFSET_MASK;

			//get the upper 8 bits of the first 16 bits
      //by shifting then masking
		int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;

    //look up the corresponding physical page of
    //given logical page
		int physical_page = TLB_lookup(logical_page);

		//if physical page is returned, it is a TLB-hit
		if (physical_page != -1){
      //update hits constant
			hits++;
    }
		else{ // if the tlb misses
      //look up the phyiscal value from the page table
			physical_page = page_table[logical_page];
	     		//if a fault occures by not being in the page table
			if (physical_page == -1){
        //update fault count
				faults++;
        //mark the physical_page the next free page location
				physical_page = unallocated_page;
        //update free page location
				unallocated_page++;
				//copy page from backing file into physical memory space
        //at the location of the memory array +
        //number of pages and at the backing store space
				memcpy(

					memory + physical_page * PAGE_SIZE ,
					mapped_backing + logical_page * PAGE_SIZE ,
					PAGE_SIZE
				);

				page_table[logical_page] = physical_page;
			}

			TLB_addition(logical_page, physical_page);
		}

		int physical_address = (physical_page << OFFSET_BITS) | offset;
		int8_t value = memory[physical_page * PAGE_SIZE + offset];
		fprintf(

			stdout ,
			"Virtual address: %d Physical address: %d Value: %d\n" ,
			logical_address ,
			physical_address ,
			value
		);
	}
	fprintf(
		stdout,

		"Number of Translated Addresses = %d\n\
		Page Faults = %d\n\
		Page Fault Rate = %.3f\n\
		TLB Hits = %d\n\
		TLB Hit Rate = %.3f\n" ,

		total ,
		faults ,
		faults / (1.0 * total) ,
		hits ,
		hits / (1.0 * total)
	);
  //cleanup
  //after finishing the program free the mapped file
  //and close any files open
  if ( munmap(mapped_backing, MEMORY_SIZE) == -1 ){
	   perror("Error un-mmapping the file");
  }

	close(backing_store_descriptor);
	fclose(address_list);

	exit(EXIT_SUCCESS);
}

//get physical_address  from the TLB if it exists, -1 otherwise
int TLB_lookup(uint8_t logical_address_page){
	unsigned int i;
	for (i = MAX((TLB_index-TLB_SIZE),0); i < TLB_index; i++){
		struct TLB_Entry_* entry = 	&( TLB[i % TLB_SIZE] );

		if (entry->logical_address == logical_address_page){
			return entry->physical_address;
		}
	}
	return -1;
}

//add to the TLB in FIFO fashion
void TLB_addition(uint8_t logical_address, uint8_t physical_address){
	struct TLB_Entry_* entry = &( TLB[TLB_index % TLB_SIZE] );
	TLB_index++;
	entry->logical_address = logical_address;
	entry->physical_address = physical_address;
	return;
}
