#include "os.h"

/*
 * Helper function for search_pt and update_ppn
 * calculating the address for the next page frame
 * with the vpn offset
 */
uint64_t* calc_frame_addr(uint64_t pt, uint64_t vpn, int level) {
	uint64_t offset, *va;
	int shift = 36 - (9 * level);

	offset = (vpn >> shift) & 0x1ff;
	va = (uint64_t *)phys_to_virt(pt << 12) + offset;

	return va;
}

/*
 * Helper function for search_pt
 * creates a new page frame
 */
uint64_t create_pt(uint64_t* va, uint64_t pt) {
	uint64_t new_pt, pte;

	new_pt = alloc_page_frame();
	pte = (new_pt << 12) | 0x1;
	*va = pte;

	return pte;
}


/*
 * Helper function for page_table_update and page_table_query
 * searching the page table for the vpn
 * if required, creating a new pt frame
 */
uint64_t search_pt(uint64_t pt, uint64_t vpn, int level, int update) {
	uint64_t next_addr;
	uint64_t* va = calc_frame_addr(pt, vpn, level);

	next_addr = *va;
	if(!(next_addr & 0x1)) {
		if(!update) {
			return NO_MAPPING;
		} else {
			next_addr = create_pt(va, pt);
		}
	}

	return next_addr >> 12;
}


/*
 * Helper function for page_table_update
 * creates a new mapping to the given ppn
 */
void update_ppn(uint64_t pt, uint64_t vpn, uint64_t ppn) {
	uint64_t pte;
	uint64_t* va = calc_frame_addr(pt, vpn, 4);

	if (ppn == NO_MAPPING) {
		*va = 0;
	} else {
		pte = (ppn << 12) | 0x1;
		*va = pte;
	}		
}


/*
 * Create/destroy virtual memory mappings in the page table
 */
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
	int level = 0;
	uint64_t pt_addr = pt, next = NO_MAPPING;

	while(level < 4) {
		next = search_pt(pt_addr, vpn, level, 1);
		if((next == NO_MAPPING) && (ppn == NO_MAPPING)) {
			return;
		}
		++level;
		pt_addr = next;
	}

	update_ppn(pt_addr, vpn, ppn);
}


/*
 * Query the mapping of a virtual page number in the page table
 */
uint64_t page_table_query(uint64_t pt, uint64_t vpn) {
	int level = 0;
	uint64_t pt_addr = pt, next = NO_MAPPING;

	while(level < 5) {
		next = search_pt(pt_addr, vpn, level, 0);
		if(next == NO_MAPPING) {
			return NO_MAPPING;
		}
		++level;
		pt_addr = next;
	}

	return pt_addr;
}
