#########################################################################
# Makefile for MicrosoftËs MPEG VM software encoder & decoder
#
# Peter Van Beek, Wed Feb  5 14:20:17 1997
# University of Rochester, Department of Electrical Engineering
#########################################################################

########## General definitions ##########################################
#CCC 		= g++ -D __TRACE_AND_STATS_ -D__DOUBLE_PRECISION_
CCC 		= g++ -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_

CFLAGS		= $(INC) -O3 -Wall -Wpointer-arith -fexceptions -fno-rtti -Wall -Werror

LN 			= /bin/ln -s
MV 			= /bin/mv
CP 			= /bin/cp
RM 			= /bin/rm -f
INSTALL		= install



########## Module definitions ###########################################
TOPDIR		= .
BINDIR		= ./bin
INSTALLBINDIR	= /usr/local/bin

GROUP		= .

SRCDIR		= $(TOPDIR)/$(GROUP)
APPDIR		= $(TOPDIR)/$(GROUP)/app
APPENCDIR	= $(TOPDIR)/$(GROUP)/app/encoder
APPDECDIR	= $(TOPDIR)/$(GROUP)/app/decoder
APPCONVDIR	= $(TOPDIR)/$(GROUP)/app/convertpar
SYSDIR		= $(TOPDIR)/$(GROUP)/sys
SYSENCDIR	= $(TOPDIR)/$(GROUP)/sys/encoder
SYSDECDIR	= $(TOPDIR)/$(GROUP)/sys/decoder
TOOLSDIR	= $(TOPDIR)/$(GROUP)/tools
TOOLSENTDIR	= $(TOPDIR)/$(GROUP)/tools/entropy
TYPEDIR		= $(TOPDIR)/$(GROUP)/type
VTCDIR		= $(TOPDIR)/$(GROUP)/vtc
VTCINCDIR	= $(TOPDIR)/$(GROUP)/vtc/include
VTCMAINDIR	= $(TOPDIR)/$(GROUP)/vtc/main
VTCPEZWDIR	= $(TOPDIR)/$(GROUP)/vtc/pezw
VTCWAVLTDIR	= $(TOPDIR)/$(GROUP)/vtc/wavelet
VTCZTEDIR	= $(TOPDIR)/$(GROUP)/vtc/zte
VTCZTQDIR	= $(TOPDIR)/$(GROUP)/vtc/ztq

OBJDIR		= $(TOPDIR)/$(GROUP)/obj

LIBS		= -lm -lobj

PROGRAMS 	= bin/mp4venc bin/mp4vdec

OBJECTS	= \
			$(OBJDIR)/block.o \
			$(OBJDIR)/paramset.o \
			$(OBJDIR)/cae.o \
			$(OBJDIR)/dct.o \
			$(OBJDIR)/error.o \
			$(OBJDIR)/mb.o \
			$(OBJDIR)/mc.o \
			$(OBJDIR)/mcpad.o \
			$(OBJDIR)/mode.o \
			$(OBJDIR)/mv.o \
			$(OBJDIR)/shape.o \
			$(OBJDIR)/spt.o \
			$(OBJDIR)/tps_bfshape.o \
			$(OBJDIR)/tps_enhcbuf.o \
			$(OBJDIR)/vopses.o \
			$(OBJDIR)/blkenc.o \
			$(OBJDIR)/errenc.o \
			$(OBJDIR)/mbenc.o \
			$(OBJDIR)/mbinterlace.o \
			$(OBJDIR)/mcenc.o \
			$(OBJDIR)/motest.o \
			$(OBJDIR)/mvenc.o \
			$(OBJDIR)/padenc.o \
			$(OBJDIR)/sesenc.o \
			$(OBJDIR)/shpenc.o \
			$(OBJDIR)/sptenc.o \
			$(OBJDIR)/vopmbenc.o \
			$(OBJDIR)/vopseenc.o \
			$(OBJDIR)/blkdec.o \
			$(OBJDIR)/errdec.o \
			$(OBJDIR)/mbdec.o \
			$(OBJDIR)/mbheaddec.o \
			$(OBJDIR)/mbinterlacedec.o \
			$(OBJDIR)/mcdec.o \
			$(OBJDIR)/mvdec.o \
			$(OBJDIR)/rvlcdec.o \
			$(OBJDIR)/shpdec.o \
			$(OBJDIR)/sptdec.o \
			$(OBJDIR)/vopmbdec.o \
			$(OBJDIR)/vopsedec.o \
			$(OBJDIR)/bitstrm.o \
			$(OBJDIR)/huffman.o \
			$(OBJDIR)/basic.o \
			$(OBJDIR)/geom.o \
			$(OBJDIR)/grayc.o \
			$(OBJDIR)/grayf.o \
			$(OBJDIR)/grayi.o \
			$(OBJDIR)/svd.o \
			$(OBJDIR)/typeapi.o \
			$(OBJDIR)/vop.o \
			$(OBJDIR)/warp.o \
			$(OBJDIR)/yuvac.o \
			$(OBJDIR)/yuvai.o \
			$(OBJDIR)/computePSNR.o \
			$(OBJDIR)/read_image.o \
			$(OBJDIR)/seg.o \
			$(OBJDIR)/vtcdec.o \
			$(OBJDIR)/vtcenc.o \
			$(OBJDIR)/wavelet.o \
			$(OBJDIR)/write_image.o \
			$(OBJDIR)/PEZW_ac.o \
			$(OBJDIR)/PEZW_utils.o \
			$(OBJDIR)/download_filter.o \
			$(OBJDIR)/dwt.o \
			$(OBJDIR)/dwt_aux.o \
			$(OBJDIR)/dwtmask.o \
			$(OBJDIR)/idwt.o \
			$(OBJDIR)/idwt_aux.o \
			$(OBJDIR)/imagebox.o \
			$(OBJDIR)/ac.o \
			$(OBJDIR)/bitpack.o \
			$(OBJDIR)/ztscanUtil.o \
			$(OBJDIR)/ztscan_dec.o \
			$(OBJDIR)/ztscan_enc.o \
			$(OBJDIR)/QMInit.o \
			$(OBJDIR)/QMUtils.o \
			$(OBJDIR)/Utils.o \
			$(OBJDIR)/decQM.o \
			$(OBJDIR)/encQM.o \
			$(OBJDIR)/errorHandler.o \
			$(OBJDIR)/msg.o \
			$(OBJDIR)/quant.o \
			$(OBJDIR)/tm5rc.o \
			$(OBJDIR)/PEZW_textureLayerBQ.o \
			$(OBJDIR)/wvtpezw_tree_init_encode.o \
			$(OBJDIR)/wvtpezw_tree_encode.o \
			$(OBJDIR)/wvtPEZW.o \
			$(OBJDIR)/wvtpezw_tree_init_decode.o \
			$(OBJDIR)/wvtpezw_tree_decode.o \
			$(OBJDIR)/PEZW_globals.o

#			$(OBJDIR)/context.o \
			#$(OBJDIR)/wvtdecodepezw_spatial_snr.o \
			#$(OBJDIR)/wvtpezw_spatial_snr.o \

# Include all subdirectories for ease
INC	= 	-I$(TOPDIR)/$(GROUP)/ \
		-I$(TOPDIR)/$(GROUP)/sys/ \
		-I$(TOPDIR)/$(GROUP)/sys/encoder \
		-I$(TOPDIR)/$(GROUP)/sys/decoder \
		-I$(TOPDIR)/$(GROUP)/type \
		-I$(TOPDIR)/$(GROUP)/tools \
		-I$(TOPDIR)/$(GROUP)/tools/entropy \
		-I$(TOPDIR)/$(GROUP)/vtc \
		-I$(TOPDIR)/$(GROUP)/vtc/include \
		-I$(TOPDIR)/$(GROUP)/vtc/main \
		-I$(TOPDIR)/$(GROUP)/vtc/pezw \
		-I$(TOPDIR)/$(GROUP)/vtc/wavelet \
		-I$(TOPDIR)/$(GROUP)/vtc/zte \
		-I$(TOPDIR)/$(GROUP)/vtc/ztq \
		-L$(OBJDIR)


.KEEP_STATE:

########## Targets #########################################

$(GROUP): $(OBJECTS)
	@echo Done making $(GROUP) objects

all:	$(OBJECTS) $(PROGRAMS)
	@echo Done making $(GROUP) objects and programs

# -------- encoder ---------------------------------------------

bin/mp4venc: $(OBJECTS) 
	@echo $(CCC) $(CFLAGS) $(INC) -o $(BINDIR)/mp4venc $(APPENCDIR)/encoder.cpp $(LIBS)
	@$(CCC) $(CFLAGS) $(INC) -o $(BINDIR)/mp4venc $(APPENCDIR)/encoder.cpp $(LIBS)
	@echo Done making mp4venc

# -------- decoder ---------------------------------------------

bin/mp4vdec: $(OBJECTS) 
	@echo $(CCC) $(CFLAGS) $(INC) -o $(BINDIR)/mp4vdec $(APPDECDIR)/decoder.cpp $(LIBS)
	@$(CCC) $(CFLAGS) $(INC) -o $(BINDIR)/mp4vdec $(APPDECDIR)/decoder.cpp $(LIBS)
	@echo Done making mp4vdec

# -------- decoder ---------------------------------------------

bin/convertpar: $(OBJECTS) 
	@echo $(CCC) $(CFLAGS) $(INC) -o $(BINDIR)/convertpar $(APPCONVDIR)/convertpar.cpp $(LIBS)
	@$(CCC) $(CFLAGS) $(INC) -o $(BINDIR)/convertpar $(APPCONVDIR)/convertpar.cpp $(LIBS)
	@echo Done making convertpar

# -------- necessary Objects --------------------------
# -------- with generic rules -------------------------

#$(OBJDIR)/%.o: $(APPDIR)/%.cpp
#	$(CCC) $(CFLAGS)  -c $< -o $@

$(OBJDIR)/%.o: $(APPENCDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@

$(OBJDIR)/%.o: $(APPDECDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@

$(OBJDIR)/%.o: $(APPCONVDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@


$(OBJDIR)/%.o: $(SYSDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(SYSENCDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(SYSDECDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(TOOLSDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(TOOLSENTDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(TYPEDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(VTCMAINDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(VTCPEZWDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(VTCWAVLTDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(VTCZTEDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

$(OBJDIR)/%.o: $(VTCZTQDIR)/%.cpp
	$(CCC) $(CFLAGS)  -c $< -o $@
	ar rcv $(OBJDIR)/libobj.a $@

# =========================================================================
# ----- and now install... ------------------------------------

install: install-exec

install-exec:	$(PROGRAMS)
	$(INSTALL) $(BINDIR)/mp4venc $(INSTALLBINDIR)
	$(INSTALL) $(BINDIR)/mp4vdec $(INSTALLBINDIR)
	@echo Installed $(PROGRAMS) in $(INSTALLBINDIR)

# ----- ... and deinstall if you are unsatisfied --------------

deinstall:	
	$(RM) $(INSTALLBINDIR)/mp4venc
	$(RM) $(INSTALLBINDIR)/mp4vdec
	@echo Removed $(PROGRAMS) from $(INSTALLBINDIR)

# ----- clean the obj-Subdiir ---------------------------------

clean:
	$(RM) core $(PROGRAMS) $(OBJECTS) $(OBJDIR)/*.o $(OBJDIR)/libobj.a @echo Done cleaning ... 
