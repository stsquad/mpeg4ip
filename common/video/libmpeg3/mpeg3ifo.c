#include <byteswap.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "ifo.h"
#include "mpeg3private.h"
#include "mpeg3protos.h"

typedef struct
{
	int64_t start_byte;
	int64_t end_byte;
// Used in final table
	int program;
// Used in cell play info
	int cell_type;
// Used in cell addresses
	int vob_id;
	int cell_id;
} mpeg3ifo_cell_t;

typedef struct
{
	mpeg3ifo_cell_t *cells;
	long total_cells;
	long cells_allocated;
} mpeg3ifo_celltable_t;


#define CADDR_HDR_LEN 8

typedef struct {
	u_short	num		: 16;	// Number of Video Objects
	u_short unknown		: 16;	// don't know
	u_int	len		: 32;	// length of table
} cell_addr_hdr_t;

typedef struct {
	u_int	foo		: 16;	// ???
	u_int	num		: 16;	// number of subchannels
} audio_hdr_t;

#define AUDIO_HDR_LEN 4

typedef struct {
	u_short id		: 16;	// Language
	u_short			: 16;	// don't know
	u_int	start		: 32;	// Start of unit
} pgci_sub_t;

#define PGCI_SUB_LEN 8

#define PGCI_COLOR_LEN 4


static u_int get4bytes(u_char *buf)
{
	return bswap_32 (*((u_int32_t *)buf));
}

static u_int get2bytes(u_char *buf)
{
	return bswap_16 (*((u_int16_t *)buf));
}

static int ifo_read(int fd, long pos, long count, unsigned char *data)
{
	if((pos = lseek(fd, pos, SEEK_SET)) < 0)
	{
    	perror("ifo_read");
    	return -1;
    }

	return read(fd, data, count); 
}

#define OFF_PTT get4bytes (ifo->data[ID_MAT] + 0xC8)
#define OFF_TITLE_PGCI get4bytes (ifo->data[ID_MAT] + 0xCC)
#define OFF_MENU_PGCI get4bytes (ifo->data[ID_MAT] + 0xD0)
#define OFF_TMT get4bytes (ifo->data[ID_MAT] + 0xD4)
#define OFF_MENU_CELL_ADDR get4bytes (ifo->data[ID_MAT] + 0xD8)
#define OFF_MENU_VOBU_ADDR_MAP get4bytes (ifo->data[ID_MAT] + 0xDC)
#define OFF_TITLE_CELL_ADDR get4bytes (ifo->data[ID_MAT] + 0xE0)
#define OFF_TITLE_VOBU_ADDR_MAP get4bytes (ifo->data[ID_MAT] + 0xE4)

#define OFF_VMG_TSP get4bytes (ifo->data[ID_MAT] + 0xC4)
#define OFF_VMG_MENU_PGCI get4bytes (ifo->data[ID_MAT] + 0xC8)
#define OFF_VMG_TMT get4bytes (ifo->data[ID_MAT] + 0xD0)


static int ifo_vts(ifo_t *ifo)
{
    if(!strncmp((char*)ifo->data[ID_MAT], "DVDVIDEO-VTS", 12)) 
		return 0;

	return -1;
}


static int ifo_vmg(ifo_t *ifo)
{
    if(!strncmp((char*)ifo->data[ID_MAT], "DVDVIDEO-VMG", 12))
		return 0;

	return -1;
}

static int ifo_table(ifo_t *ifo, unsigned long offset, unsigned long tbl_id)
{
	unsigned char *data;
	unsigned long len = 0;
	int i;
	u_int32_t *ptr;

	if(!offset) return -1;

	data = (u_char *)calloc(1, DVD_VIDEO_LB_LEN);

	if(ifo_read(ifo->fd, ifo->pos + offset * DVD_VIDEO_LB_LEN, DVD_VIDEO_LB_LEN, data) <= 0) 
	{
		perror("ifo_table");
		return -1;
	}

	switch(tbl_id)
	{
		case ID_TITLE_VOBU_ADDR_MAP:
		case ID_MENU_VOBU_ADDR_MAP:
			len = get4bytes(data) + 1;
			break;

		default: 
		{
			ifo_hdr_t *hdr = (ifo_hdr_t *)data;
			len = bswap_32(hdr->len) + 1;
		}
	}

	if(len > DVD_VIDEO_LB_LEN) 
	{
		data = (u_char *)realloc((void *)data, len);
		bzero(data, len);
		ifo_read(ifo->fd, ifo->pos + offset * DVD_VIDEO_LB_LEN, len, data);
	}

	ifo->data[tbl_id] = data;
	ptr = (u_int32_t*)data;
	len /= 4;

	if(tbl_id == ID_TMT) 
		for (i = 0; i < len; i++)
			ptr[i] = bswap_32(ptr[i]);

	return 0;
}

static ifo_t* ifo_open(int fd, long pos)
{
	ifo_t *ifo;

	ifo = (ifo_t *)calloc(sizeof(ifo_t), 1);

	ifo->data[ID_MAT] = (unsigned char *)calloc(DVD_VIDEO_LB_LEN, 1);

	ifo->pos = pos; 
	ifo->fd = fd;

	if(ifo_read(fd, pos, DVD_VIDEO_LB_LEN, ifo->data[ID_MAT]) < 0) 
	{
		perror("ifo_open");
		free(ifo->data[ID_MAT]);
		free(ifo);
		return NULL;
	}

	ifo->num_menu_vobs	= get4bytes(ifo->data[ID_MAT] + 0xC0);
	ifo->vob_start		= get4bytes(ifo->data[ID_MAT] + 0xC4);

#ifdef DEBUG
	printf ("num of vobs: %x vob_start %x\n", ifo->num_menu_vobs, ifo->vob_start);
#endif

	if(!ifo_vts(ifo)) 
	{
		ifo_table(ifo, OFF_PTT, ID_PTT);
    	ifo_table(ifo, OFF_TITLE_PGCI, ID_TITLE_PGCI);
		ifo_table(ifo, OFF_MENU_PGCI, ID_MENU_PGCI);
		ifo_table(ifo, OFF_TMT, ID_TMT);
		ifo_table(ifo, OFF_MENU_CELL_ADDR, ID_MENU_CELL_ADDR);
		ifo_table(ifo, OFF_MENU_VOBU_ADDR_MAP, ID_MENU_VOBU_ADDR_MAP);
    	ifo_table(ifo, OFF_TITLE_CELL_ADDR, ID_TITLE_CELL_ADDR);
		ifo_table(ifo, OFF_TITLE_VOBU_ADDR_MAP, ID_TITLE_VOBU_ADDR_MAP);
	} 
	else 
	if(!ifo_vmg(ifo)) 
	{
		ifo_table(ifo, OFF_VMG_TSP, ID_TSP);
		ifo_table(ifo, OFF_VMG_MENU_PGCI, ID_MENU_PGCI);
		ifo_table(ifo, OFF_VMG_TMT, ID_TMT);
		ifo_table(ifo, OFF_TITLE_CELL_ADDR, ID_TITLE_CELL_ADDR);
		ifo_table(ifo, OFF_TITLE_VOBU_ADDR_MAP, ID_TITLE_VOBU_ADDR_MAP);
	}

	return ifo;
}


static int ifo_close(ifo_t *ifo)
{
	if(ifo->data[ID_MAT]) free(ifo->data[ID_MAT]);
	if(ifo->data[ID_PTT]) free(ifo->data[ID_PTT]);
	if(ifo->data[ID_TITLE_PGCI]) free(ifo->data[ID_TITLE_PGCI]);
	if(ifo->data[ID_MENU_PGCI]) free(ifo->data[ID_MENU_PGCI]);
	if(ifo->data[ID_TMT]) free(ifo->data[ID_TMT]);
	if(ifo->data[ID_MENU_CELL_ADDR]) free(ifo->data[ID_MENU_CELL_ADDR]);
	if(ifo->data[ID_MENU_VOBU_ADDR_MAP]) free(ifo->data[ID_MENU_VOBU_ADDR_MAP]);
	if(ifo->data[ID_TITLE_CELL_ADDR]) free(ifo->data[ID_TITLE_CELL_ADDR]);
	if(ifo->data[ID_TITLE_VOBU_ADDR_MAP]) free(ifo->data[ID_TITLE_VOBU_ADDR_MAP]);

	free(ifo);

	return 0;
}

static int ifo_audio(char *_hdr, char **ptr)
{
	audio_hdr_t *hdr = (audio_hdr_t *)_hdr;

	if(!_hdr) return -1;

	*ptr = _hdr + AUDIO_HDR_LEN;

	return bswap_16(hdr->num);
}


static int pgci(ifo_hdr_t *hdr, int title, char **ptr)
{
	pgci_sub_t *pgci_sub;

	*ptr = (char *) hdr;

	if(!*ptr) return -1;

	if(title > hdr->num) return -1;

	*ptr += IFO_HDR_LEN;

	pgci_sub = (pgci_sub_t *)*ptr + title;

	*ptr = (char *)hdr + bswap_32(pgci_sub->start);

	return 0;
}

static int program_map(char *pgc, char **ptr)
{
	int num;
	*ptr = pgc;

	if (!pgc)
		return -1;

	*ptr += 2;	
	num = **ptr;

	*ptr += 10;
	*ptr += 8 * 2;			// AUDIO
	*ptr += 32 * 4;			// SUBPICTURE
	*ptr += 8;
	*ptr += 16 * PGCI_COLOR_LEN;	// CLUT
	*ptr += 2;

	*ptr = get2bytes((unsigned char*)*ptr) + pgc;

	return num;
}


static u_int get_cellplayinfo(u_char *pgc, u_char **ptr)
{
	u_int num;
	*ptr = pgc;

	if (!pgc)
		return -1;

	*ptr += 3;	
	num = **ptr;

	*ptr += 9;
	*ptr += 8 * 2;			// AUDIO
	*ptr += 32 * 4;			// SUBPICTURE
	*ptr += 8;
	*ptr += 16 * PGCI_COLOR_LEN;	// CLUT
	*ptr += 4;

	*ptr =  get2bytes(*ptr) + pgc;

	return num;
}

static void get_ifo_playlist(mpeg3_t *file, mpeg3_demuxer_t *demuxer)
{
	DIR *dirstream;
	char directory[MPEG3_STRLEN];
	char filename[MPEG3_STRLEN];
	char complete_path[MPEG3_STRLEN];
	char title_path[MPEG3_STRLEN];
	char vob_prefix[MPEG3_STRLEN];
	struct dirent *new_filename;
	char *ptr;
	int64_t total_bytes = 0;
	int done = 0, i;

// Get titles matching ifo file
	mpeg3io_complete_path(complete_path, file->fs->path);
	mpeg3io_get_directory(directory, complete_path);
	mpeg3io_get_filename(filename, complete_path);
	strncpy(vob_prefix, filename, 6);

	dirstream = opendir(directory);
	while(new_filename = readdir(dirstream))
	{
		if(!strncasecmp(new_filename->d_name, vob_prefix, 6))
		{
			ptr = strrchr(new_filename->d_name, '.');
			if(ptr && !strncasecmp(ptr, ".vob", 4))
			{
// Got a title
				if(atol(&new_filename->d_name[7]) > 0)
				{
					mpeg3_title_t *title;

					mpeg3io_joinpath(title_path, directory, new_filename->d_name);
					title = demuxer->titles[demuxer->total_titles++] = mpeg3_new_title(file, title_path);
					title->total_bytes = mpeg3io_path_total_bytes(title_path);
					total_bytes += title->total_bytes;
//printf("%s\n", title_path);
				}
			}
		}
	}


// Alphabetize titles.  Only problematic for guys who rip entire DVD's
// to their hard drives while retaining the file structure.
	while(!done)
	{
		done = 1;
		for(i = 0; i < demuxer->total_titles - 1; i++)
		{
			if(strcmp(demuxer->titles[i]->fs->path, demuxer->titles[i + 1]->fs->path) > 0)
			{
				mpeg3_title_t *temp = demuxer->titles[i];
				demuxer->titles[i] = demuxer->titles[i + 1];
				demuxer->titles[i + 1] = temp;
				done = 0;
			}
		}
	}
	
	
}




// IFO parsing
static void get_ifo_header(mpeg3_demuxer_t *demuxer, ifo_t *ifo)
{
	int i;
// Video header
	demuxer->vstream_table[0] = 1;

// Audio header
	if(!ifo_vts(ifo))
	{
		ifo_audio_t *audio;
		int result = 0;
// Doesn't detect number of tracks.
		int atracks = ifo_audio((char*)ifo->data[ID_MAT] + IFO_OFFSET_AUDIO, (char**)&audio);
		int atracks_empirical = 0;

// Construct audio stream id's
#define TEST_START 0x600000
#define TEST_LEN   0x100000
		mpeg3demux_open_title(demuxer, 0);
		mpeg3demux_seek_byte(demuxer, TEST_START);
		while(!result && 
			!mpeg3demux_eof(demuxer) && 
			mpeg3demux_tell(demuxer) < TEST_START + TEST_LEN)
		{
			result = mpeg3_read_next_packet(demuxer);
		}
		mpeg3demux_seek_byte(demuxer, 0);

		for(i = 0; i < MPEG3_MAX_STREAMS; i++)
		{
//printf("%x %d\n", i, demuxer->astream_table[i]);
			if(demuxer->astream_table[i]) atracks_empirical++;
		}

// Doesn't detect PCM audio or total number of tracks
/*
 * 		if(atracks && !atracks_empirical)
 * 			for(i = 0; i < atracks; i++)
 * 			{
 * 				int audio_mode = AUDIO_AC3;
 * 				switch(audio->coding_mode)
 * 				{
 * 					case 0: audio_mode = AUDIO_AC3;  break;
 * 					case 1: audio_mode = AUDIO_MPEG; break;
 * 					case 2: audio_mode = AUDIO_MPEG; break;
 * 					case 3: audio_mode = AUDIO_PCM;  break;
 * 				}
 * 				if(!demuxer->astream_table[i + 0x80]) demuxer->astream_table[i + 0x80] = audio_mode;
 * 			}
 */
	}
	else
	if(!ifo_vmg(ifo))
	{
	}
}

static mpeg3ifo_cell_t* append_cell(mpeg3ifo_celltable_t *table)
{
	if(!table->cells || table->total_cells >= table->cells_allocated)
	{
		long new_allocation;
		mpeg3ifo_cell_t *new_cells;

		new_allocation = table->cells_allocated ? table->cells_allocated * 2 : 64;
		new_cells = calloc(1, sizeof(mpeg3ifo_cell_t) * new_allocation);
		if(table->cells)
		{
			memcpy(new_cells, table->cells, sizeof(mpeg3ifo_cell_t) * table->total_cells);
			free(table->cells);
		}
		table->cells = new_cells;
		table->cells_allocated = new_allocation;
	}

	return &table->cells[table->total_cells++];
}

static void delete_celltable(mpeg3ifo_celltable_t *table)
{
	if(table->cells) free(table->cells);
	free(table);
}

static void cellplayinfo(ifo_t *ifo, mpeg3ifo_celltable_t *cells)
{
	int i, j;
	char *cell_hdr, *cell_hdr_start, *cell_info;
	ifo_hdr_t *hdr = (ifo_hdr_t*)ifo->data[ID_TITLE_PGCI];
	int program_chains = bswap_16(hdr->num);
	long total_cells;

//printf("cellplayinfo\n");
	for(j = 0; j < program_chains; j++)
	{
// Cell header
		pgci(hdr, j, &cell_hdr);
		cell_hdr_start = cell_hdr;
// Unknown
		cell_hdr += 2;
// Num programs
		cell_hdr += 2;
// Chain Time
		cell_hdr += 4;
// Unknown
		cell_hdr += 4;
// Subaudio streams
		for(i = 0; i < 8; i++) cell_hdr += 2;
// Subpictures
		for(i = 0; i < 32; i++) cell_hdr += 4;
// Unknown
		for(i = 0; i < 8; i++) cell_hdr++;
// Skip CLUT
// Skip PGC commands
// Program map
		if(program_map(cell_hdr_start, &cell_hdr))
			;

// Cell Positions
		if(total_cells = get_cellplayinfo((unsigned char*)cell_hdr_start, (unsigned char**)&cell_hdr))
		{
//printf("cellplayinfo %d %d\n", j, total_cells);
			cell_info = cell_hdr;
			for(i = 0; i < total_cells; i++)
			{
				ifo_pgci_cell_addr_t *cell_addr = (ifo_pgci_cell_addr_t *)cell_info;
				long start_byte = bswap_32(cell_addr->vobu_start);
				long end_byte = bswap_32(cell_addr->vobu_last_end);
				int cell_type = cell_addr->chain_info;

				if(!cells->total_cells && start_byte > 0)
					start_byte = 0;

				if(!cells->total_cells ||
					end_byte >= cells->cells[cells->total_cells - 1].end_byte)
				{
					mpeg3ifo_cell_t *cell = append_cell(cells);

					cell->start_byte = start_byte;
					cell->end_byte = end_byte;
					cell->cell_type = cell_type;
//printf("cellplayinfo start: %llx end: %llx type: %x\n", 
//	(int64_t)cell->start_byte * 0x800, (int64_t)cell->end_byte * 0x800, cell->cell_type);
				}
				cell_info += PGCI_CELL_ADDR_LEN;
			}
		}
	}
}

static void celladdresses(ifo_t *ifo, mpeg3ifo_celltable_t *cell_addresses)
{
	int i;
	char *ptr = (char*)ifo->data[ID_TITLE_CELL_ADDR];
	int total_addresses;
	cell_addr_hdr_t *cell_addr_hdr = (cell_addr_hdr_t*)ptr;
	ifo_cell_addr_t *cell_addr = (ifo_cell_addr_t*)(ptr + CADDR_HDR_LEN);
	int done = 0;
//printf("celladdresses\n");

	if(total_addresses = bswap_32(cell_addr_hdr->len) / sizeof(ifo_cell_addr_t))
	{
		for(i = 0; i < total_addresses; i++)
		{
			mpeg3ifo_cell_t *cell;
			cell = append_cell(cell_addresses);
			cell->start_byte = (int64_t)bswap_32(cell_addr->start);
			cell->end_byte = (int64_t)bswap_32(cell_addr->end);
			cell->vob_id = bswap_16(cell_addr->vob_id);
			cell->cell_id = cell_addr->cell_id;
/*
 * printf("celladdresses vob id: %x cell id: %x start: %ld end: %ld\n", 
 * 	bswap_16(cell_addr->vob_id), cell_addr->cell_id, (long)cell->start_byte, (long)cell->end_byte);
 */
			cell_addr++;
		}
	}

// Sort addresses by address instead of vob id
	done = 0;
	while(!done)
	{
		done = 1;
		for(i = 0; i < total_addresses - 1; i++)
		{
			mpeg3ifo_cell_t *cell1, *cell2;
			cell1 = &cell_addresses->cells[i];
			cell2 = &cell_addresses->cells[i + 1];

			if(cell1->start_byte > cell2->start_byte)
			{
				mpeg3ifo_cell_t temp = *cell1;
				*cell1 = *cell2;
				*cell2 = temp;
				done = 0;
				break;
			}
		}
	}
	
	for(i = 0; i < total_addresses; i++)
	{
		mpeg3ifo_cell_t *cell = &cell_addresses->cells[i];
//printf("celladdresses vob id: %x cell id: %x start: %ld end: %ld\n", 
//	cell->vob_id, cell->cell_id, (long)cell->start_byte, (long)cell->end_byte);
	}
}

static void finaltable(mpeg3ifo_celltable_t *final_cells, 
		mpeg3ifo_celltable_t *cells, 
		mpeg3ifo_celltable_t *cell_addresses)
{
	int input_cell = 0, current_address = 0;
	int output_cell = 0;
	int done;
	int i, j;
	int current_vobid;
// Start and end bytes of programs
	long program_start_byte[256], program_end_byte[256];

	final_cells->total_cells = 0;
	final_cells->cells_allocated = cell_addresses->total_cells;
	final_cells->cells = calloc(1, sizeof(mpeg3ifo_cell_t) * final_cells->cells_allocated);

// Assign programs to cells
	current_vobid = -1;
	for(i = cell_addresses->total_cells - 1; i >= 0; i--)
	{
		mpeg3ifo_cell_t *input = &cell_addresses->cells[i];
		mpeg3ifo_cell_t *output = &final_cells->cells[i];
		if(current_vobid < 0) current_vobid = input->vob_id;

		*output = *input;
// Reduce current vobid
		if(input->vob_id < current_vobid)
			current_vobid = input->vob_id;
		else
// Get the current program number
		if(input->vob_id > current_vobid)
		{
			int current_program = input->vob_id - current_vobid;
			output->program = current_program;

// Get the last interleave by brute force
			for(j = i; 
				j < cell_addresses->total_cells && cell_addresses->cells[i].cell_id == cell_addresses->cells[j].cell_id; 
				j++)
			{
				int new_program = final_cells->cells[j].vob_id - current_vobid;
				if(new_program <= current_program)
					final_cells->cells[j].program = new_program;
			}
		}

		final_cells->total_cells++;
	}

// Expand byte position and remove duplicates
	for(i = 0; i < final_cells->total_cells; i++)
	{
		if(i < final_cells->total_cells - 1 &&
			final_cells->cells[i].start_byte == final_cells->cells[i + 1].start_byte)
		{
			for(j = i; j < final_cells->total_cells - 1; j++)
				final_cells->cells[j] = final_cells->cells[j + 1];

			final_cells->total_cells--;
		}

		final_cells->cells[i].start_byte *= (int64_t)2048;
		final_cells->cells[i].end_byte *= (int64_t)2048;
	}

return;
// Debug
	printf("finaltable\n");
	for(i = 0; i < final_cells->total_cells; i++)
	{
		printf(" vob id: %x cell id: %x start: %llx end: %llx program: %x\n", 
			final_cells->cells[i].vob_id, final_cells->cells[i].cell_id, (int64_t)final_cells->cells[i].start_byte, (int64_t)final_cells->cells[i].end_byte, final_cells->cells[i].program);
	}
}




/* Read the title information from a ifo */
static int read_ifo(mpeg3_t *file, 
	mpeg3_demuxer_t *demuxer, 
	int read_timecodes)
{
	int64_t last_ifo_byte = 0, first_ifo_byte = 0;
	mpeg3ifo_celltable_t *cells, *cell_addresses, *final_cells;
	int current_title = 0, current_cell = 0;
	int i;
	ifo_t *ifo;
    int fd = mpeg3io_get_fd(file->fs);
	int64_t title_start_byte = 0;
	int result;

//printf("read_ifo 1\n");
	if(!(ifo = ifo_open(fd, 0))) 
	{
		fprintf(stderr, "read_ifo: Error decoding ifo.\n");
		return 1;
	}

//printf("read_ifo 1\n");
//	file->packet_size = 2048;
	demuxer->read_all = 1;
	cells = calloc(1, sizeof(mpeg3ifo_celltable_t));
	cell_addresses = calloc(1, sizeof(mpeg3ifo_celltable_t));
	final_cells = calloc(1, sizeof(mpeg3ifo_celltable_t));

//printf("read_ifo 1\n");
	get_ifo_playlist(file, demuxer);
	get_ifo_header(demuxer, ifo);
	cellplayinfo(ifo, cells);
	celladdresses(ifo, cell_addresses);
	finaltable(final_cells, 
		cells, 
		cell_addresses);

//printf("read_ifo 2\n");
// Assign cells to titles
	while(final_cells && current_cell < final_cells->total_cells)
	{
		mpeg3_title_t *title;
		mpeg3ifo_cell_t *cell;
		int64_t cell_start, cell_end;
		int64_t length = 1;

		title = demuxer->titles[current_title];
		cell = &final_cells->cells[current_cell];
		cell_start = cell->start_byte;
		cell_end = cell->end_byte;

//printf("read_ifo 1 %d %llx %llx %d\n", current_cell, (int64_t)cell->start_byte, (int64_t)cell->end_byte, cell->program);
		while(cell_start < cell_end && length)
		{
			length = cell_end - cell_start;

			if(cell_start + length - title_start_byte > title->total_bytes)
				length = title->total_bytes - cell_start + title_start_byte;
//printf("read_ifo 3 %llx - %llx + %llx = %llx\n", title->total_bytes, cell_start, title_start_byte, length);


// Should never fail.  If it does it means the length of the cells and the
// length of the titles don't match.  The title lengths must match or else
// the cells won't line up.
			if(length)
			{
				mpeg3_new_timecode(title, 
					(long)(cell_start - title_start_byte), 
					0,
					(long)(cell_start - title_start_byte + length),
					0,
					cell->program);

				cell_start += length;
			}
			else
			{
				fprintf(stderr, "read_ifo: cell length and title length don't match.\n");
			}

// Advance title
			if(cell_start - title_start_byte >= title->total_bytes && 
				current_title < demuxer->total_titles - 1)
			{
				title_start_byte += title->total_bytes;
				title = demuxer->titles[++current_title];
			}
		}
		current_cell++;
	}

//printf("read_ifo 4\n");
// Look up time values for the timecodes
// Should only be used for building a TOC
	if(read_timecodes)
	{
		for(current_title = 0; current_title < demuxer->total_titles; current_title++)
		{
			mpeg3_title_t *title = demuxer->titles[current_title];
			mpeg3demux_open_title(demuxer, current_title);

			for(i = 0; i < title->timecode_table_size; i++)
			{
				mpeg3demux_timecode_t *timecode = &title->timecode_table[i];

				mpeg3io_seek(title->fs, timecode->start_byte);
				mpeg3_read_next_packet(demuxer);
				timecode->start_time = demuxer->time;

				mpeg3io_seek(title->fs, timecode->end_byte);
				if(timecode->end_byte >= title->total_bytes)
					mpeg3_read_prev_packet(demuxer);
				else
					mpeg3_read_next_packet(demuxer);

				timecode->end_time = demuxer->time;
			}
		}
		mpeg3demux_open_title(demuxer, 0);
	}

//for(i = 0; i < demuxer->total_titles; i++) mpeg3_dump_title(demuxer->titles[i]);

	delete_celltable(cells);
	delete_celltable(cell_addresses);
	delete_celltable(final_cells);
	ifo_close(ifo);

//printf("read_ifo 5\n");
	mpeg3demux_assign_programs(demuxer);
//printf("read_ifo 6\n");
	return 0;
}

int mpeg3_read_ifo(mpeg3_t *file, int read_timecodes)
{
	file->is_program_stream = 1;

	read_ifo(file, file->demuxer, read_timecodes);
	return 0;
}

