#include "quicktime.h"

#define DEFAULT_INFO "Made with Quicktime for Linux"

int quicktime_udta_init(quicktime_udta_t *udta)
{
	udta->copyright = 0;
	udta->copyright_len = 0;
	udta->name = 0;
	udta->name_len = 0;

	udta->info = malloc(strlen(DEFAULT_INFO) + 1);
	udta->info_len = strlen(DEFAULT_INFO);
	sprintf(udta->info, DEFAULT_INFO);

	quicktime_hnti_init(&(udta->hnti));

	return 0;
}

int quicktime_udta_delete(quicktime_udta_t *udta)
{
	if(udta->copyright_len)
	{
		free(udta->copyright);
	}
	if(udta->name_len)
	{
		free(udta->name);
	}
	if(udta->info_len)
	{
		free(udta->info);
	}
	quicktime_hnti_delete(&(udta->hnti));

	quicktime_udta_init(udta);
	return 0;
}

int quicktime_udta_dump(quicktime_udta_t *udta)
{
	printf(" user data (udta)\n");
	if(udta->copyright_len) printf("  copyright -> %s\n", udta->copyright);
	if(udta->name_len) printf("  name -> %s\n", udta->name);
	if(udta->info_len) printf("  info -> %s\n", udta->info);
	quicktime_hnti_dump(&udta->hnti);
}

int quicktime_read_udta(quicktime_t *file, quicktime_udta_t *udta, quicktime_atom_t *udta_atom)
{
	quicktime_atom_t leaf_atom;
	int result = 0;

	do
	{
		/* udta atoms can be terminated by a 4 byte zero */
		if (udta_atom->end - quicktime_position(file) < HEADER_LENGTH) {
			u_char trash[HEADER_LENGTH];
			int remainder = udta_atom->end - quicktime_position(file);
			quicktime_read_data(file, trash, remainder);
			break;
		}

		quicktime_atom_read_header(file, &leaf_atom);
		
		if(quicktime_atom_is(&leaf_atom, "©cpy"))
		{
			result += quicktime_read_udta_string(file, &(udta->copyright), &(udta->copyright_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, "©nam"))
		{
			result += quicktime_read_udta_string(file, &(udta->name), &(udta->name_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, "©inf"))
		{
			result += quicktime_read_udta_string(file, &(udta->info), &(udta->info_len));
		}
		else if (quicktime_atom_is(&leaf_atom, "hnti")) {
			quicktime_read_hnti(file, &(udta->hnti), &leaf_atom);
		}
		else
		quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < udta_atom->end);

	return result;
}

int quicktime_write_udta(quicktime_t *file, quicktime_udta_t *udta)
{
	quicktime_atom_t atom, subatom;

	/*
	 * Empty udta atom makes Darwin Streaming Server unhappy
	 * so avoid it
	 */
	if (file->use_mp4) {
		if (udta->copyright_len == 0
		  && udta->hnti.rtp.string == NULL) {
			return;
		}
	} else {
		if (udta->copyright_len + udta->name_len + udta->info_len == 0
		  && udta->hnti.rtp.string == NULL) {
			return;
		}
	}

	quicktime_atom_write_header(file, &atom, "udta");

	if(udta->copyright_len)
	{
		quicktime_atom_write_header(file, &subatom, "©cpy");
		quicktime_write_udta_string(file, udta->copyright, udta->copyright_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->name_len && !file->use_mp4)
	{
		quicktime_atom_write_header(file, &subatom, "©nam");
		quicktime_write_udta_string(file, udta->name, udta->name_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->info_len && !file->use_mp4)
	{
		quicktime_atom_write_header(file, &subatom, "©inf");
		quicktime_write_udta_string(file, udta->info, udta->info_len);
		quicktime_atom_write_footer(file, &subatom);
	}
	if (udta->hnti.rtp.string != NULL) {
		quicktime_write_hnti(file, &(udta->hnti));
	}

	quicktime_atom_write_footer(file, &atom);
}

int quicktime_read_udta_string(quicktime_t *file, char **string, int *size)
{
	int result;

	if(*size) free(*string);
	*size = quicktime_read_int16(file);  /* Size of string */
	quicktime_read_int16(file);  /* Discard language code */
	*string = malloc(*size + 1);
	result = quicktime_read_data(file, *string, *size);
	(*string)[*size] = 0;
	return !result;
}

int quicktime_write_udta_string(quicktime_t *file, char *string, int size)
{
	int new_size = strlen(string);
	int result;

	quicktime_write_int16(file, new_size);    /* String size */
	quicktime_write_int16(file, 0);    /* Language code */
	result = quicktime_write_data(file, string, new_size);
	return !result;
}

int quicktime_set_udta_string(char **string, int *size, char *new_string)
{
	if(*size) free(*string);
	*size = strlen(new_string + 1);
	*string = malloc(*size + 1);
	strcpy(*string, new_string);
	return 0;
}
