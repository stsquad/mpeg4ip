
## Makefile.am for the main SDL library

# These are the subdirectories that are always built

# These are the subdirectories which may be built
EXTRA_SUBDIRS =			\
	 audio 

# These are the subdirectories which will be built now
SUBDIRS = @SDL_EXTRADIRS@

# These are the subdirectories which will be distributed with "make dist"
DIST_SUBDIRS = $(EXTRA_SUBDIRS)

# The SDL library target
lib_LTLIBRARIES = libmpeg4ipSDL.la

libmpeg4ipSDL_la_SOURCES = $(GENERAL_SRCS)
libmpeg4ipSDL_la_LDFLAGS = 		\
	-no-undefined		\
        -release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libmpeg4ipSDL_la_LIBADD =		\
	@SDL_EXTRALIBS@		\
	@SYSTEM_LIBS@
libmpeg4ipSDL_la_DEPENDENCIES =	\
	@SDL_EXTRALIBS@

# The SDL library sources
GENERAL_SRCS = 		

EXTRA_DIST = SDL_error.c
