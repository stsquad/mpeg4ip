#include "quicktime.h"



int quicktime_hdlr_init(quicktime_hdlr_t *hdlr)
{
	hdlr->version = 0;
	hdlr->flags = 0;
	hdlr->component_type[0] = 'm';
	hdlr->component_type[1] = 'h';
	hdlr->component_type[2] = 'l';
	hdlr->component_type[3] = 'r';
	hdlr->component_subtype[0] = 'v';
	hdlr->component_subtype[1] = 'i';
	hdlr->component_subtype[2] = 'd';
	hdlr->component_subtype[3] = 'e';
	hdlr->component_manufacturer = 0;
	hdlr->component_flags = 0;
	hdlr->component_flag_mask = 0;
	strcpy(hdlr->component_name, "Linux Media Handler");
}

int quicktime_hdlr_init_video(quicktime_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 'v';
	hdlr->component_subtype[1] = 'i';
	hdlr->component_subtype[2] = 'd';
	hdlr->component_subtype[3] = 'e';
	strcpy(hdlr->component_name, "Linux Video Media Handler");
}

int quicktime_hdlr_init_audio(quicktime_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 's';
	hdlr->component_subtype[1] = 'o';
	hdlr->component_subtype[2] = 'u';
	hdlr->component_subtype[3] = 'n';
	strcpy(hdlr->component_name, "Linux Sound Media Handler");
}

int quicktime_hdlr_init_hint(quicktime_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 'h';
	hdlr->component_subtype[1] = 'i';
	hdlr->component_subtype[2] = 'n';
	hdlr->component_subtype[3] = 't';
	strcpy(hdlr->component_name, "Linux Hint Media Handler");
}

int quicktime_hdlr_init_data(quicktime_hdlr_t *hdlr)
{
	hdlr->component_type[0] = 'd';
	hdlr->component_type[1] = 'h';
	hdlr->component_type[2] = 'l';
	hdlr->component_type[3] = 'r';
	hdlr->component_subtype[0] = 'a';
	hdlr->component_subtype[1] = 'l';
	hdlr->component_subtype[2] = 'i';
	hdlr->component_subtype[3] = 's';
	strcpy(hdlr->component_name, "Linux Alias Data Handler");
}

int quicktime_hdlr_delete(quicktime_hdlr_t *hdlr)
{
}

int quicktime_hdlr_dump(quicktime_hdlr_t *hdlr)
{
	printf("   handler reference (hdlr)\n");
	printf("    version %d\n", hdlr->version);
	printf("    flags %d\n", hdlr->flags);
	printf("    component_type %c%c%c%c\n", hdlr->component_type[0], hdlr->component_type[1], hdlr->component_type[2], hdlr->component_type[3]);
	printf("    component_subtype %c%c%c%c\n", hdlr->component_subtype[0], hdlr->component_subtype[1], hdlr->component_subtype[2], hdlr->component_subtype[3]);
	printf("    component_name %s\n", hdlr->component_name);
}

int quicktime_read_hdlr(quicktime_t *file, quicktime_hdlr_t *hdlr)
{
	hdlr->version = quicktime_read_char(file);
	hdlr->flags = quicktime_read_int24(file);
	quicktime_read_char32(file, hdlr->component_type);
	quicktime_read_char32(file, hdlr->component_subtype);
	hdlr->component_manufacturer = quicktime_read_int32(file);
	hdlr->component_flags = quicktime_read_int32(file);
	hdlr->component_flag_mask = quicktime_read_int32(file);
	if (file->use_mp4) {
		// TBD read null terminated string
	} else {
		quicktime_read_pascal(file, hdlr->component_name);
	}
}

int quicktime_write_hdlr(quicktime_t *file, quicktime_hdlr_t *hdlr)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "hdlr");

	quicktime_write_char(file, hdlr->version);
	quicktime_write_int24(file, hdlr->flags);

	if (file->use_mp4) {
		int i;
		quicktime_write_int32(file, 0x00000000);
		quicktime_write_char32(file, hdlr->component_subtype);
		for (i = 0; i < 3; i++) {
			quicktime_write_int32(file, 0x00000000);
		}
		quicktime_write_data(file, hdlr->component_name, 
			strlen(hdlr->component_name) + 1);
	} else {
		quicktime_write_char32(file, hdlr->component_type);
		quicktime_write_char32(file, hdlr->component_subtype);
		quicktime_write_int32(file, hdlr->component_manufacturer);
		quicktime_write_int32(file, hdlr->component_flags);
		quicktime_write_int32(file, hdlr->component_flag_mask);
		quicktime_write_pascal(file, hdlr->component_name);
	}

	quicktime_atom_write_footer(file, &atom);
}
