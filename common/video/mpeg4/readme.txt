PLEASE READ THE ISO COPYRIGHT AGREEMENT (header in each source file) BEFORE PROCEEDING.


----------------------------------------------------------


VIDEO CODEC

Directories:
type:	 data type object classes 
tools:	 tool classes
sys:	 codec system classes
app:	 application, i.e., codec
example: sample bitstream and parameter file

Compile:
If you are using Microsoft VC++5.x compiler, 
use "encoder.dsp" and "decoder.dsp" in app\encoder or app\decoder

Files needed for video encoder:
1. All the *.cpp files under type\
2. All the *.cpp files under tools\
3. All the *.cpp files under sys\
4. All the *.cpp files under sys\encoder\
5. app\encoder\encoder.cpp

Files needed for video decoder:
1. All the *.cpp files under type\
2. All the *.cpp files under tools\
3. All the *.cpp files under sys\
4. All the *.cpp files under sys\decoder\
5. app\decoder\decoder.cpp

Files needed for the convertpar utility:
1. convertpar.cpp in app\convertpar

Tested Compilers:
		The following compilers are known to compile the source code without problem -
		Microsoft Visual C++ 5.x
		GNU g++ (version?)

Compiler Options:
		Define __TRACE_AND_STATS_ to generate statitics and trace information (encoder only).
		Define __PC_COMPILER_ to run it on Windows.
		Define __DOUBLE_PRECISION_ to use double precision for DCT and IDCT.
	Hyundai: Define __NBIT_ to use more than 8 bits of video. e.g. 12 bit pixels.
        Toshiba: Define __NEW_CBPY_ if you want to use the new CBPY tables.

If NBIT is defined, then the input and output YUV files require 16 bits per pixel instead of 8.


Run Time

Demo parameter files and bitstreams as well as further instructions about running the software are
found in demo\.

To run encoder:   encoder [parameter_file_name]

An example parameter file can be found in app\encoder\example.par

To run decoder:   decoder [bitstream_file_name] [reconstructed_image_file_name] [display_with] [display_height]

For example, "decoder example.cmp example_recon 176 144" will decoder the bitstream file "example.cmp"
and save the reconstructed images in "example_recon.yuv" as 176x144 images.

When you decoded spatial scalable bitstreams, set to arguments as the following:

"decoder [baselayer bitsream] [enhancement layer bitstream] [output yuv filename] [baselayer width] [baselayer height]"

The frame size of the enhancement layer is set at the same value as vol height and width,


----------------------------------------------------------


VISUAL TEXTURE CODEC

Directories:
All directories within vtc

To encode VTC, set the VTC flag in the parameter file, and provide a .ctl file for the wavelet codec.

To decode VTC streams, use the following command line:

decoder -vtc bitstream_file output_file target_spatial_layer target_snr_layer


----------------------------------------------------------


Contact:         Simon Winder (swinder@microsoft.com)
                 Ming-Chieh Lee (mingcl@microsoft.com)
		 Wei-ge Chen (wchen@microsoft.com)
		 Microsoft Corporation
		 One Microsoft Way
		 Redmond, WA 98052

