# Dummy configuration for GLIB - comment it out, basically

AC_DEFUN(AM_PATH_GLIB,
   [ dnl
     no_glib=yes
     GLIB_CFLAGS=""
     GLIB_LIBS=""
     AC_SUBST(GLIB_CFLAGS)
     AC_SUBST(GLIB_LIBS)
   ])

AC_DEFUN(AM_PATH_GTK,
   [ dnl
     no_gtk=yes
     GTK_CFLAGS=""
     GTK_LIBS=""
     AC_SUBST(GTK_CFLAGS)
     AC_SUBST(GTK_LIBS)
   ])
