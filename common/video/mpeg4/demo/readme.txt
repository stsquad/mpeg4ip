Microsoft Visual FDIS Software Version 1.0
August 12th 1999


1. FILES

This package contains the following files:

encoder.exe		Video encoder
decoder.exe		Video decoder
readme.txt		Info file
example.par		Example parameter file
stef_cif.par		Example parameter file
brea_cif.par		Example parameter file
sprt_stef_sif.par	Example sprite parameter file
stef_cif.cmp		Example encoded bitstream (frame-based coding)
brea_cif.cmp		Example encoded bitstream (object-based coding)
sprt_stef_sif.cmp	Example sprite bitstream
vtc_brea_cif.cmp	Example Visual Texture bitstream
vtc_brea_cif.ctl	Example Visual Texture control file
vtc_brea_cif.par	Example Visual Texture parameter file


2. ENCODING A SEQUENCE

Prepare a parameter file to control the encoder operation. The file example.par can be used as a
template for this. It will be necessary to adjust the width and height fields to the size of your
source image, change the first and last frame numbers appropriately, and set the temporal sampling
rate. If the temporal sampling rate is set to 3, every third frame will be encoded. Additionally,
the I, P, or B picture sequence can be controlled with the "Number of PVOP's between 2 IVOP's" and
"Number of BVOP's between 2 PVOP's" fields. The source file name should be included in the "file
prefix" field, and the directory path to that file given in "file directory". For object-based
encoding, the encoder inserts the object index number between the directory path and file prefix.
The source file must end with the .yuv extension and be in 4:2:0 raw YUV format. For object-based
coding, a segmentation mask file must also be present with a .seg extension. This should contain
binary mask or alpha-plane mask data, where a transparent pixel has value 0, and an opaque pixel
is represented by 255. The .yuv and .seg files are simple dumps of binary data, with frames
concatenated together with no format information. For .yuv files, each frame consists of width *
height Y pixels then width/2 * height/2 U pixels then width/2 * height/2 V pixels. For the .seg
file there each frame consists of width * height Alpha pixels.

Run the encoder using the following command line:

	encoder par_file.par

where par_file.par is the name of the parameter file.

The encoder will generate a bitstream file, which is placed in .\cmp, and a reconstructed output
file, which is placed in .\rec. These locations can be altered by changing the relevant fields in
the parameter file.

2A. PARAMETER FILES

The parameter file format has changed. Files with the old format (before version 901) can be
converted to the new format by running the convertpar tool which is included with the release.

convertpar old_par_file [new_par_file]

"[xxx]" means optional.


3. DECODING A SEQUENCE

The decoder takes a bitstream file and decodes it, generating a .yuv file and, if object-based
decoding is enabled, also a .seg file. The following command line should be used:

	decoder bit_file.cmp out_file base_width base_height

where bit_file.cmp is the name of the encoded bitstream file, and out_file is the root name of
the output file to create. The .yuv or .seg extensions will be added by the decoder. Here, base_width
and base_height represent the width and height of the destination image. This can be larger than the
encoded image size if necessary.

To decode a bitstream with an enhancement layer, use the command line:

	decoder bit_file.cmp bit_file_enh.cmp out_file base_width base_height

where bit_file_enh.cmp is the name of the enhancement layer bitstream file.


4. EXAMPLES

The example bitstreams can be decoded using the following command lines:

decoder stef_cif.cmp stef_cif 352 288
decoder brea_cif.cmp brea_cif 352 288
decoder sprt_stef_sif.cmp sprt_stef_cif 352 240
decoder -vtc vtc_brea_cif.cmp out.yuv 1 1

5. PROBLEMS

The encoder and decoder are release compiled, the error reporting is bad, as most things are trapped
with assert statements that are removed for the release version. There will be access violations if,
for example, the correct parameters are not specified...

6. RESOURCES

For information about the software contained in this package, contact:

Ming-Chieh Lee (mingcl@microsoft.com)
Wei-ge Chen (wchen@microsoft.com)
Simon Winder (swinder@microsoft.com)

		 Microsoft Corporation
		 One Microsoft Way
		 Redmond, WA 98052
