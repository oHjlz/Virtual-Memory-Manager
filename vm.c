#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10
#define PAGE_TABLE_SIZE 256
#define NUMBER_OF_FRAMES 256
#define TLB_SIZE 16

//* Define a type alias for struct tlb_entry */
typedef struct tlb_entry {
    int page_number;
    int frame_number;
    int valid;
} tlb_entry_t;

FILE *address_file;
FILE *backing_store;

/* buffer for reading logical addresses */
char buffer[BUFFER_SIZE];

/* representation of physical memory */
signed char *physical_memory;

/* page table */
int page_table[PAGE_TABLE_SIZE];

/* list of free frames */
int free_frame_list[NUMBER_OF_FRAMES];

/* the TLB */
tlb_entry_t tlb[TLB_SIZE];

int next_free_frame = 0;
int tlb_index = 0;

int page_faults = 0;
int tlb_hits = 0;

int search_tlb(int page_number){
    for(int i = 0; i < TLB_SIZE; i++){

        if(tlb[i].page_number == page_number){
            tlb_hits++;
            return tlb[i].frame_number; //TLB hit
        }
    }
    return -1;   //TLB miss
}

void add_to_tlb(int page, int frame){
    tlb[tlb_index].page_number = page;
    tlb[tlb_index].frame_number = frame;
    tlb[tlb_index].valid = 1;
    tlb_index = (tlb_index + 1) % TLB_SIZE;
    //circular buffer, emulating FIFO 

}

int get_free_frame(){
    int frame = next_free_frame;
    next_free_frame = (next_free_frame + 1) % NUMBER_OF_FRAMES;
    //circular, to emulate FIFO
    return frame;
}


int main(int argc, char *argv[]){
// Open the backing store (binary file)
    backing_store = fopen("BACKING_STORE.bin", "rb");
    if (!backing_store) {
        printf("Error opening BACKING_STORE.bin\n");
        return 1;
    }

// Allocating physical memory
    //signed char = 1 Byte, so array of 65,536 bytes == total virtual adress space
    physical_memory = (signed char *)malloc(NUMBER_OF_FRAMES * 256);
    if(physical_memory != NULL){
        //printf("This isnt NULL");
    }

// Initializing the page table
    for(int i = 0; i < PAGE_TABLE_SIZE; i++){
        page_table[i] = -1;
    }
    // for(int i = 0; i < PAGE_TABLE_SIZE; i++){
    //     printf("%d\n", page_table[i]);
    // }

//Flushing the TLB
    for(int i = 0; i < TLB_SIZE; i++){
        tlb[i].valid = 0;
    }
    // for(int i = 0; i < TLB_SIZE; i++){
    //     printf("%d\n", tlb[i].valid);
    // }

// Open the addresses file (text file)
    address_file = fopen("addresses.txt", "r");
    if (!address_file) {
        printf("Error opening addresses.txt\n");
        fclose(backing_store);
        return 1;
    }
//logic to get the physical address
    while (fgets(buffer, BUFFER_SIZE, address_file) != NULL) {
        int logical_address = atoi(buffer);
        //atoi converts strings to integers

        //shift 8 bits right to get rid of the offfset, and with FF to keep the 
        //last8 bits, which are the page number
        int page_number = (logical_address >> 8) & 0xFF;

        //offset is last 8 bits, and with FF to end up with the offset
        int offset = (logical_address & 0xFF);
        
        int frame_number = search_tlb(page_number);

        if(frame_number == -1){ 
            //not in TLB check the page table
            frame_number = page_table[page_number];
            if(frame_number == -1){
                page_faults++;
                // Page fault occurred, not in the page table

                // Find a free frame
                int free_frame = get_free_frame();

                // Seek to page in backing store
                fseek(backing_store, page_number * 256, SEEK_SET);

                // Load page into physical memory
                fread(physical_memory + (free_frame * 256), sizeof(signed char), 256,
                 backing_store);

                // Update page table
                page_table[page_number] = free_frame;

                // Add to TLB
                add_to_tlb(page_number, free_frame);

                frame_number = free_frame;
            }
        }
        
        int physical_address = (frame_number * 256) + offset;
        signed char value = physical_memory[physical_address];

        printf("Virtual address: %d Physical address: %d Value: %d \n", logical_address, physical_address, value);
        //printf("%d\n", logical_address);
        //printf("%d\n", page_number);
        //printf("%d\n", offset);
        
        
    }
    
    double tlb_hit_rate = (tlb_hits / 1000.0);
    //printf("%d",tlb_hits);
    printf("Page Faults = %d\nTLB Hit Rate = %f\n ", page_faults, tlb_hit_rate);

    fclose(address_file);
    fclose(backing_store);
    return 0;
}