
#include "Dialogs.r"
#include "Menus.r"


resource 'DLOG' (1000) {
	{114, 64, 219, 428},
	dBoxProc,
	visible,
	noGoAway,
	0x0,
	1000,
	"Command Line",
	centerMainScreen
};

resource 'DITL' (1000) {
	{
		// The OK button
		{70, 288, 90, 346},
		Button {
			enabled,
			"OK"
		},
		// The Cancel button
		{70, 210, 90, 268},
		Button {
			enabled,
			"Cancel"
		},
		// The command line edit
		{40, 40, 56, 325},
		EditText {
			enabled,
			""
		},
		{70,20,88,126},
		CheckBox {
			enabled,
			"Output to file"
		},
		// A little explantation
		{16, 10, 32, 325},
		StaticText {
			enabled,
			"Command Line:"
		}
	}
};

resource 'DLOG' (1001) {
{114, 64, 219, 428},
	dBoxProc,
	visible,
	noGoAway,
	0x0,
	1001,
	"Error Window",
	centerMainScreen

};

resource 'DITL' (1001) {
	{
		// The OK button
		{70, 288, 90, 346},
		Button {
			enabled,
			"OK"
		},
		//where to stick the bad news
		{16,10,56,352},
		StaticText {
			enabled,
			""
		}
	}
};


resource 'MENU' (128, preload) {
	128,
	textMenuProc,
	0x7FFFFFFD,
	enabled,
	apple,
	{ "About SDL...", noIcon, noKey, noMark, plain,
	  "-", noIcon, noKey, noMark, plain
	}
};


//the following data structures are for resedit, so it knows the format
//of the CLne interface, which will allow the programer to edit the stored
//command line using a GUI instead of a Hex Editor
type 'TMPL'
{
	array dataTypes {
		pstring;
		longint;
	};
};


resource 'TMPL' (128, "CLne") 
{
	{
		"Command Line", 'PSTR',
		"Save To File", 'BOOL'
	}
};

// Here are resources for the DSp resolution switch confirmation dialog (whew!)
// I don't have a fancy tool to create nice rez code from rsrc files...here is hex
data 'DLOG' (1002) {
	$"00B8 00BE 0147 01D8 0005 0100 0000 0000 0000 03EA 1643 6F6E 6669 726D 2044 6973"                    /* .¸.¾.G.Ø...........ê.Confirm Dis */
	$"706C 6179 2043 6861 6E67 6510 280A"                                                                 /* play Change.(Â */
};

data 'DITL' (1002) {
	$"0002 0000 0000 006F 001E 0083 0058 0406 4361 6E63 656C 0000 0000 006E 00C0 0082"                    /* .......o...ƒ.X..Cancel.....n.À.‚ */
	$"00FA 0402 4F4B 0000 0000 000E 000F 005F 010C 88B3 5468 6520 7365 7474 696E 6720"                    /* .ú..OK........._..ˆ³The setting  */
	$"666F 7220 796F 7572 206D 6F6E 6974 6F72 2068 6173 2062 6565 6E20 6368 616E 6765"                    /* for your monitor has been change */
	$"642C 2061 6E64 2069 7420 6D61 7920 6E6F 7420 6265 2064 6973 706C 6179 6564 2063"                    /* d, and it may not be displayed c */
	$"6F72 7265 6374 6C79 2E20 546F 2063 6F6E 6669 726D 2074 6865 2064 6973 706C 6179"                    /* orrectly. To confirm the display */
	$"2069 7320 636F 7272 6563 742C 2063 6C69 636B 204F 4B2E 2054 6F20 7265 7475 726E"                    /*  is correct, click OK. To return */
	$"2074 6F20 7468 6520 6F72 6967 696E 616C 2073 6574 7469 6E67 2C20 636C 6963 6B20"                    /*  to the original setting, click  */
	$"4361 6E63 656C 2E00"                                                                                /* Cancel.. */
};