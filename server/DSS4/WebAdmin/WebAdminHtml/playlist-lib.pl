#!/usr/bin/perl
#----------------------------------------------------------
#
# @APPLE_LICENSE_HEADER_START@
#
# Copyright (c) 1999-2001 Apple Computer, Inc.  All Rights Reserved. The
# contents of this file constitute Original Code as defined in and are
# subject to the Apple Public Source License Version 1.2 (the 'License').
# You may not use this file except in compliance with the License.  Please
# obtain a copy of the License at http://www.apple.com/publicsource and
# read it before using this file.
#
# This Original Code and all software distributed under the License are
# distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please
# see the License for the specific language governing rights and
# limitations under the License.
#
#
# @APPLE_LICENSE_HEADER_END@
#
#
#---------------------------------------------------------

# playlist-lib.pl
# Common functions for handling playlist broadcaster files
#
# ALL YOUR BASE ARE BELONG TO US

package playlistlib;

# -------------------------------------------------
# GetGenreOptions($selectedItemName, $defaultItem, \@genreArrayRef)
#
# Encode a playlist name into our internal format.
#
# returns encoded name.
# -------------------------------------------------
sub GetGenreOptions {
	my $selectedItemName = $_[0];
	my $defaultItem = $_[1];
	my $genreArrayRef = $_[2];
	my @genreArray = @$genreArrayRef;
	my $str = '';
	my $item = '';
	
	foreach $item (@genreArray) {
		$str .= "\t\t<option value=\"$item\"";
		if (($item eq $selectedItemName) or (($selectedItemName eq '') and ($item eq $defaultItem))) {
			$str .= ' selected';
		}
		$str .= ">$item</option>\r";
	}
	return $str;
}

# -------------------------------------------------
# EncodePLName(name)
#
# Encode a playlist name into our internal format.
#
# returns encoded name.
# -------------------------------------------------
sub EncodePLName {
    my $name = $_[0];

    # convert whitespace to underscore
    $name =~ s/[ \t]/_/g;

    # convert double quote to escape sequence
    $name =~ s/\"/%22/g;

    # convert single quote to escape sequence
    $name =~ s/\'/%27/g;

    # convert dot to escape sequence
    $name =~ s/\./%2e/g;

    # convert slash to escape sequence
    $name =~ s/\//%2f/g;
    
    # drop any non-alphanumeric chars
    $name =~ s/[^0-9a-zA-Z]//g;

    return $name;
}

# -------------------------------------------------
# DecodePLName(name)
#
# Decode a playlist name from our internal format.
#
# returns decoded name.
# -------------------------------------------------
sub DecodePLName {
    my $name = $_[0];

    # convert underscore or plus to whitespace
    $name =~ s/[+_]/ /g;

	# convert the hex characters back to ASCII
    $name =~ s/%(..)/pack("c",hex($1))/ge;

    return $name;
}

# -------------------------------------------------
# ParentDir(dirname)
#
# return the parent directory of the given dirname.
#
# returns parent directory name.
# -------------------------------------------------
sub ParentDir {
    my $dir = $_[0];
    my $parent = "";

    if ($dir eq "/") {
    	# root directory's parent is itself
        return $dir;
    }
    elsif ($dir =~ /(.+)[\/]$/) {
        # remove trailing slash if it's there
        $dir = $1;
    }
    if ($dir =~ /^[\/][^\/]+$/) {
    	# one below root, root is our parent
        return "/";
    }
    # find the parent directory
	$dir =~ /(.+)[\/][^\/]+$/;
	$parent = $1;
    return $parent;
}

# -------------------------------------------------
# PushCurrPlayList(name)
#
# Save the current encoded playlist name so we can
# recover it later.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub PushCurrPlayList {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $targ = "$plroot" . "broadcast_curname";

    if (!open(NAMFILE, "> $targ")) {
        # can't open a file to hold the current playlist name.
        return 0;
    }
    print NAMFILE "$name\n";
    close(NAMFILE);
    return 1;
}

# -------------------------------------------------
# PopCurrPlayList()
#
# Return the current encoded playlist name that we
# previously saved with PushCurrPlayList().
#
# returns name on success or "none" on failure.
# -------------------------------------------------
sub PopCurrPlayList {
    my $name = "none";
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $targ = "$plroot" . "broadcast_curname";

    if (open(NAMFILE, "< $targ")) {
        $name = <NAMFILE>;
        chop $name;
        close(NAMFILE);
    }
    return $name;
}

# -------------------------------------------------
# PushCurrPWDir(name)
#
# Save the current working dir name so we can
# recover it later.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub PushCurrPWDir {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $targ = "$plroot" . "broadcast_pwd";

    if (!open(NAMFILE, "> $targ")) {
        # can't open a file to hold the current playlist name.
        return 0;
    }
    print NAMFILE "$name\n";
    close(NAMFILE);
    return 1;
}

# -------------------------------------------------
# PopCurrPWDir()
#
# Return the current working dir name that we
# previously saved with PushCurrPWDir().
#
# returns name on success or "none" on failure.
# -------------------------------------------------
sub PopCurrPWDir {
    my $name = "none";
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $targ = "$plroot" . "broadcast_pwd";

    if (open(NAMFILE, "< $targ")) {
        $name = <NAMFILE>;
        chop $name;
        close(NAMFILE);
    }
    return $name;
}

# -------------------------------------------------
# GetPLRootDir()
#
# returns the name of the root play list directory.
# -------------------------------------------------
sub GetPLRootDir {
	my $plroot;
    my $filedelim = &playlistlib::GetFileDelimChar();

	$plroot = $ENV{"PLAYLISTS_ROOT"};

    # make sure path is terminated with platform file delimeter
    if (!($plroot =~ /\/$/) && !($plroot =~ /\\$/)) {
        $plroot .= $filedelim;
    }

    return $plroot;
}

# -------------------------------------------------
# SortPlayList(\@order_array, \@pl_name_array)
#
# Sort a playlist name array according to the integer
# order array.
#
# returns reference to sorted list.
# -------------------------------------------------
sub SortPlayList {
    my $ordref = $_[0];
    my $plaref = $_[1];
    my $n = scalar @$ordref;
    my $tmp1;
    my $tmp2;
    my $i;
    my $j;
    #
    # simple insertion sort algorithm
    #
    for ($i=1; $i<$n; $i+=1) {
        $tmp1 = $$ordref[$i];
        $tmp2 = $$plaref[$i];
        $j = $i-1;
        while (($j>-1)&&($$ordref[$j]>$tmp1)) {
            $$ordref[$j+1] = $$ordref[$j];
            $$plaref[$j+1] = $$plaref[$j];
            $j-=1;
        }
        $$ordref[$j+1] = $tmp1;
        $$plaref[$j+1] = $tmp2;
    }
    return $plaref;
}

# -------------------------------------------------
# RemoveMovieFromList(\@mlist, $movie)
#
# returns the list minus the movie removed.
# -------------------------------------------------
sub RemoveMovieFromList {
    my $arref = $_[0];
    my $movie = $_[1];
    my @mlist = ();
    my $x;

    foreach $x (@$arref) {
        $x =~ /^(.+)[:]/;
        if ($movie eq $1) {
            #this matches so we don't append it;
        }
        else {
            # no match so append to list
            push @mlist, $x;
        }
	}
    return \@mlist;
}

# -------------------------------------------------
# GetFileDelimChar ()
#
# returns the correct file delimeter character for 
# this platform.
# -------------------------------------------------
sub GetFileDelimChar {
	my $filedelim;
    if($^O ne "MSWin32") {
        $filedelim = "/";
    }
    else {
        $filedelim = "\\";
    }
    return $filedelim;
}

# -------------------------------------------------
# EmitPlayListErrLogFile(name)
#
# Output the contents of the playlist error log.
# returns the content of the error log file as HTML.
# -------------------------------------------------
sub EmitPlayListErrLogFile {
    my $name = $_[0];
    $name = &playlistlib::EncodePLName($name);
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    # setup HTML header string.
    my $htmlstr = "<tt>\n";

    if (!open(ERRFILE, "< $plroot$name$filedelim$name.err")) {
        # failed to open error log file
    	$htmlstr .= "&nbsp;Log not found!<br></tt>\n";
        return $htmlstr;
    }
    # read each line of the log file and turn it
    # into valid HTML strings.
    while(<ERRFILE>) {
    	chomp;
    	# replace special characters
    	$_ =~ s/</&lt;/g;
    	$_ =~ s/>/&gt;/g;
    	$_ =~ s/&/&amp;/g;
    	$htmlstr .= "&nbsp;&nbsp;$_<br>\n";
    }
    close(ERRFILE);
    # append HTML footer.
    $htmlstr .= "</tt>\n";

    return $htmlstr;
}

# -------------------------------------------------
# CreatePlaylistFile (name, plarref)
#
# Creates the play list file from playlist array ref.
# The filename parameter must include the path if we
# are not in the pwd.
# Each entry in the arry must be in the form:
#       "moviename:10" where the number is the
# play weight of the movie.
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub CreatePlaylistFile {
    my $name = $_[0]; # full pathname of PL file
    my $plarref = $_[1];
    my $movie;
    my $wt;

    if (!open(PLFILE, "> $name")) {
        # failed to create playlist file
        return 0;
    }
    print PLFILE "*PLAY-LIST*\n";
    print PLFILE "#\n";
    print PLFILE "# Created by QTSS Admin CGI Server\n";
    print PLFILE "#\n";
    foreach my $item (@$plarref) {
        $item =~ /(.+)[:]([0-9]+)$/;
        $movie = $1;
        $wt = $2;
        if ($wt == "") {
            $wt = 10;
        }
        $movie =~ s/\"/\"\"/g;
        print PLFILE "\"$movie\" $wt\n";
    }
    close (PLFILE);
    return 1;
}

# -------------------------------------------------
# ParsePlaylistFile(filename)
#
# Opens the play list file and parse it.
# The filename parameter must include the path if we
# are not in the pwd.
#
# Each entry in the array will be in the form:
#       "moviename:10" where the number is the
# play weight of the movie.
# returns array reference to the movie list.
# -------------------------------------------------
sub ParsePlaylistFile {
    my $name = $_[0]; # full pathname of PL file
    my $arg1;
    my $arg2;
    my $i = 0;
    my @movieList = ();

    if (!open(PLFILE, "< $name")) {
        # failed to open playlist file
        return \@movieList;
    }
    # first line must be: "*PLAY-LIST*"
    $_ = <PLFILE>;
    if (! /\*PLAY-LIST\*/) {
        return \@movieList;
    }
    # start with empty movie list
    # and add entries as we parse them.
    while(<PLFILE>) {
        $arg1 = "";
        $arg2 = "";
        chomp;               # no newline
        s/^#.*//;            # no comments (at beginning of line)
        s/\s+$//;            # no trailing whitespace
        next unless length;  # anything left?
        if (/([0-9]*)$/o) {
        	$arg1 = $_;
        	$arg2 = $1;
        	$arg1 =~ s/[ \t]*[0-9]*$//o;
        	$arg1 =~ s/^"//o;
        	$arg1 =~ s/"$//o;
        	$arg1 =~ s/""/"/go;
        }
        next unless length $arg1;
        if (!length $arg2) {
            # the default weight is 10.
            $arg2 = 10;
        }
        $movieList[$i] = "$arg1:$arg2";
        $i++;
    }
    close(PLFILE);
    return \@movieList;
}

# -------------------------------------------------
# CreateNewPlaylistDir(dirname, plarref)
#
# Creates the play list directory inside $plroot.
# we will also put a playlist in this directory.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub CreateNewPlaylistDir {
	my $plroot = &GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
	my $name = $_[0];
    my $plarref = $_[1];
	my $result = 0;
	
	# if the top-level playlists dir doesn't exist try and create it.
	if (!(-e "$plroot")) {
	    if ((mkdir "$plroot", 0770) == 0) {
	        return $result;
	    }
	}
	# create the playlistfile after creating the dir for it.
	if ((-e "$plroot$name") || (mkdir "$plroot$name", 0770)) {
		$result = &playlistlib::CreatePlaylistFile("$plroot$name$filedelim$name.playlist", $plarref);
	}
	return $result;
}

# -------------------------------------------------
# RemovePlaylistDir(dirname)
#
# Removes the play list directory inside $plroot.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub RemovePlaylistDir {
	my $plroot = &playlistlib::GetPLRootDir();
	my $dir = "$plroot" . "$_[0]";
    my $filedelim = &playlistlib::GetFileDelimChar();
	my $name = "$_[0]";
	my $result = 0;
	
	if (!(-e "$dir")) {
		# directory doesn't exist; nothing to delete.
		# assume this is not an error.
		return 1;
	}
	if (&playlistlib::GetPlayListState($name) == 1) {
	    &playlistlib::StopPlayList($name);
	}
	if (opendir(DIR, $dir)) {
	    while( defined ($file = readdir DIR) ) {
	    	# delete all the files in this directory.
	    	# NOTE: we assume there are no sub-directories.
	    	if ($file =~ m/[^.]/go) {
	    	    $file = "$dir$filedelim$file";
			    if (!(unlink($file))) {

					# die($!);
				    # error: couldn't delete the file.
				    closedir(DIR);
				    return 0;
				}
			}
	    }
		closedir(DIR);
		# remove the empty directory.
		$result = rmdir $dir;
	}
	return $result;
}

# -------------------------------------------------
# EmitMovieListHtml(dirname, \@labels)
#
# Generate the HTML for the movie list in dirname.
#
# returns HTML string for the movie list table.
# -------------------------------------------------
sub EmitMovieListHtml {
	my $dir = "$_[0]";
    my $lablref = $_[1];
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $htmlstr = "";
    my $fileloc = "";
	
	if (!(-e "$dir")) {
		# directory doesn't exist; nothing to list.
        $htmlstr = "\t<TR><TD>&nbsp;<br></TD><TD>$$lablref[2] \"$dir\"</TD></TR>\r\n";
		return $htmlstr;
	}
    # make sure path is terminated with platform file delimeter
    if (($dir !~ /\/$/) && ($dir !~ /\\$/)) {
        $dir .= $filedelim;
    }
	if (opendir(DIR, $dir)) {
	    while( defined ($file = readdir DIR) ) {
	        #
	    	# Look for subdirectories & files not ending with a ".sdp".
	    	#
	    	if ($file =~ /^[.]+/) {
	    		#ignore any file or directory that starts with a dot
	    	}
	    	elsif (-d "$dir$file") {
	    	
	    	    # This is a subdirectory in the current movies directory
	    	    
                $htmlstr .= "\t<TR>\r\n";

                $htmlstr .= "\t\t<TD ALIGN=center WIDTH=30 BGCOLOR=\"#EFEFF7\">";

                $htmlstr .= "<IMG SRC=\"../images/icon_folder.gif\" ONCLICK=\"setdirectory(\'$file\')\" WIDTH=16 HEIGHT=16 ALIGN=bottom ALT=\"\"></TD>\n";

                $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$file</TD>\n";

                $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$$lablref[0]</TD>\n";

                $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" ALIGN=CENTER>";

                $htmlstr .= "&nbsp;<br></TD>\n";

                $htmlstr .= "\t</TR>\n";

	    	}
	    	elsif ($file !~ /[.][Ss][Dd][Pp]$/) {
	    	
	    		my $tlabl;
	    		
	    		if (($file =~ /[.][Mm][Oo][Vv]$/) || 
	    			($file =~ /[.][Mm][Pp]3$/) || 
	    			($file =~ /[.][Mm][Ii][Dd]$/) || 
	    			($file =~ /[.][Ww][Aa][Vv]$/) || 
	    			($file =~ /[.][Aa][Vv][Ii]$/) ) {
	    			# let's assume this is a valid media file and label it as such
	    			$tlabl = $$lablref[1];
	    		}
	    		else {
	    			# we don't know if this is a valid media file
	    			$tlabl = "&nbsp;<br>";
	    		}
	    	
	    	    # This is assumed to be a media file in the current movies directory
	    	    
	    	    $fileloc = "$dir$file";
	    	    
                $htmlstr .= "\t<TR>\r\n";

                $htmlstr .= "\t\t<TD ALIGN=center WIDTH=30 BGCOLOR=\"#EFEFF7\">";

                $htmlstr .= "<IMG SRC=\"../images/icon_movie.gif\" WIDTH=16 HEIGHT=16 ALIGN=bottom ALT=\"\"></TD>\n";

                $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$file</TD>\n";

                $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$tlabl</TD>\n";

                $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" ALIGN=CENTER>";

                $htmlstr .= "<INPUT TYPE=checkbox NAME=\"addmovie\" VALUE=\"$fileloc\">&nbsp;<br></TD>\n";

                $htmlstr .= "\t</TR>\n";
			}
	    }
		closedir(DIR);
	}
	return $htmlstr;
}

# -------------------------------------------------
# EmitPLRemoveMovieTableRowHTML($name, $wt, $label)
#
# Emit one row of HTML for the remove movie playlist
# table.
# returns HTML string for the row.
# -------------------------------------------------
sub EmitPLRemoveMovieTableRowHTML {
    my $name = $_[0];
    my $wt = $_[1];
    my $label = $_[2];
    my $htmlstr = "";
    my $i;

    $htmlstr .= "\t<TR>\n";

    $htmlstr .= "\t\t<TD ALIGN=center WIDTH=30 BGCOLOR=\"#EFEFF7\">";
    $htmlstr .= "<IMG SRC=\"../images/icon_movie.gif\" WIDTH=16 HEIGHT=16 ALIGN=bottom ALT=\"\"></TD>\n";

    $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$name</TD>\n";

    $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$label</TD>\n";

    $htmlstr .= "\t\t<TD ALIGN=center BGCOLOR=\"#EFEFF7\"><INPUT TYPE=checkbox NAME=\"remove\" VALUE=\"$name\"></TD>\n";

    $htmlstr .= "\t</TR>\n"; 

    return $htmlstr;    
}

# -------------------------------------------------
# EmitPLRemoveMovieTableHtml(plarref, $label)
#
# Generate HTML for a remove movie movie table from 
# playlist array  ref.
# Each entry in the arary must be in the form:
#       "moviename:10" where the number is the
# play weight of the movie.
# returns HTML string on success or "" on failure.
# -------------------------------------------------
sub EmitPLRemoveMovieTableHtml {
    my $plarref = $_[0];
    my $label = $_[1];
    my $movie;
    my $wt;
    my $htmlstr = "";

    foreach my $item (@$plarref) {
        $item =~ /(.+)[:]([0-9]+)$/;
        $movie = $1;
        $wt = $2;
        if ($wt == "") {
            $wt = 10;
        }
        $htmlstr .= &playlistlib::EmitPLRemoveMovieTableRowHTML($movie, $wt, $label);
    }
    return $htmlstr;    
}

# -------------------------------------------------
# GeneratePLRemoveMovieTable(dirname, label)
#
# Generate HTML for the remove movie playlist page 
# using the directory name.
# returns HTML string on success or "" on failure.
# -------------------------------------------------
sub GeneratePLRemoveMovieTable {
	my $name = $_[0];
	my $label = $_[1];
	my $result = "";
	my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
	
	# parse the playlist directory and
	# generate the HTML for the movie table.
	if (-e "$plroot$name$filedelim$name.playlist") {
	    my $temp;
		$temp = &playlistlib::ParsePlaylistFile("$plroot$name$filedelim$name.playlist");
		$result = &playlistlib::EmitPLRemoveMovieTableHtml(\@$temp, $label);
	}
	return $result;
}

# -------------------------------------------------
# EmitPLDetailTableRowHTML($name, $wt, $order, $label)
#
# Emit one row of HTML movie playlist for a table.
# returns HTML string for the row.
# -------------------------------------------------
sub EmitPLDetailTableRowHTML {
    my $name = $_[0];
    my $wt = $_[1];
    my $order = $_[2];
    my $label = $_[3];
    my $htmlstr = "";
    my $i;

    if (($wt < 1) || ($wt > 10)) {
        $wt = 10;
    }

    $htmlstr .= "\t\t<TR>\n";

    $htmlstr .= "\t\t\t<TD BGCOLOR=\"#EFEFF7\" ALIGN=center>";
    $htmlstr .= "<INPUT TYPE=text NAME=\"order\" VALUE=$order SIZE=4></TD>\n";

    $htmlstr .= "\t\t\t<TD ALIGN=center BGCOLOR=\"#EFEFF7\">";
    $htmlstr .= "<IMG SRC=\"../images/icon_movie.gif\" WIDTH=16 HEIGHT=16 ALT=\"\"></TD>\n";

    $htmlstr .= "\t\t\t<TD WIDTH=\"60%\" BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$name</TD>\n";

    $htmlstr .= "\t\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$label</TD>\r\n";

    $htmlstr .= "\t\t\t<TD BGCOLOR=\"#EFEFF7\" ALIGN=center><SELECT NAME=\"weight\">\n"; 

    for ($i=1; $i <= 10; $i++) {
        if ($i == $wt) {
            $htmlstr .= "\t\t\t\t<OPTION SELECTED VALUE=\"$name:$i\" CLASS=\"smallTD\">$i\n";
        } else {
            $htmlstr .= "\t\t\t\t<OPTION VALUE=\"$name:$i\" CLASS=\"smallTD\">$i\n";
        }
    }
    $htmlstr .= "\t\t\t</SELECT></TD>\n"; 

    $htmlstr .= "\t\t</TR>\n"; 

    return $htmlstr;    
}

# -------------------------------------------------
# EmitPLDetailTableHtml(plarref, $label)
#
# Generate HTML for a detail movie table from playlist 
# array  ref.
# Each entry in the arary must be in the form:
#       "moviename:10" where the number is the
# play weight of the movie.
# returns HTML string on success or "" on failure.
# -------------------------------------------------
sub EmitPLDetailTableHtml {
    my $plarref = $_[0];
    my $label = $_[1];
    my $movie;
    my $wt;
    my $htmlstr = "";
    my $i = 1;

    foreach my $item (@$plarref) {
        $item =~ /(.+)[:]([0-9]+)$/;
        $movie = $1;
        $wt = $2;
        if ($wt == "") {
            $wt = 10;
        }
        $htmlstr .= &playlistlib::EmitPLDetailTableRowHTML($movie, $wt, $i, $label);
        $i++;
    }
    return $htmlstr;    
}

# -------------------------------------------------
# GeneratePLDetailTable(dirname, label)
#
# Generate HTML for the detail playlist page using 
# the directory name.
# returns HTML string on success or "" on failure.
# -------------------------------------------------
sub GeneratePLDetailTable {
	my $name = $_[0];
	my $label = $_[1];
	my $result = "";
	my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
	
	# parse the playlist directory and
	# generate the HTML for the movie table.
	if (-e "$plroot$name$filedelim$name.playlist") {
	    my $temp;
		$temp = &playlistlib::ParsePlaylistFile("$plroot$name$filedelim$name.playlist");
		$result = &playlistlib::EmitPLDetailTableHtml(\@$temp, $label);
	}
	return $result;
}

# -------------------------------------------------
# EmitMainPlaylistRowHTML($file, $state, $labelsref)
#
# Emit one row of HTML playlist for a table.
# returns HTML string on success or "" on failure.
# -------------------------------------------------
sub EmitMainPlaylistRowHTML {
    my $name = $_[0];
    my $dname = &playlistlib::DecodePLName($name);
    my $state = $_[1];
    my $lblsref = $_[2];
    my $htmlstr = "";

    $htmlstr .= "\t<TR>\n";

    $htmlstr .= "\t\t<TD ALIGN=center BGCOLOR=\"#EFEFF7\">";        
    $htmlstr .= "<IMG SRC=\"../images/icon_playlist.gif\" WIDTH=16 HEIGHT=16 ALT=\"\"></TD>\n";

    $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">\n";
    $htmlstr .= "\t\t\t<A HREF=\"setplsettings.cgi?cpl=$name&pg=detail\" TARGET=\"content\">";

    # here's where we print the play list name...
    $htmlstr .= "$dname</A>\n";

    $htmlstr .= "\t\t</TD>\n";

    if ($state == 0) {
    	# playlist is stopped
		$htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$$lblsref[0]</TD>\n";
		
        $htmlstr .= "\t\t<TD BGCOLOR=\"#CCCCCC\" ALIGN=CENTER><A HREF=\"startstopbcast.cgi?state=start&pl=$name\">\n";
        $htmlstr .= "\t\t\t<IMG SRC=\"../images/icon_play.gif\" WIDTH=16 HEIGHT=16 BORDER=0 ALT=\"\"></A>\n";
        $htmlstr .= "\t\t\t<IMG SRC=\"../images/icon_stop_off.gif\" WIDTH=16 HEIGHT=16 BORDER=0 ALT=\"\">\n";
    }
    elsif ($state == 1) {
    	# playlist is running
		$htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" CLASS=\"smallTD\">$$lblsref[1]</TD>\n";
		
        $htmlstr .= "\t\t<TD BGCOLOR=\"#CCCCCC\" ALIGN=CENTER>\n";
        $htmlstr .= "\t\t\t<IMG SRC=\"../images/icon_play_off.gif\" WIDTH=16 HEIGHT=16 BORDER=0 ALT=\"\">";
        $htmlstr .= "<A HREF=\"startstopbcast.cgi?state=stop&pl=$name\">\n";
        $htmlstr .= "\t\t\t<IMG SRC=\"../images/icon_stop.gif\" WIDTH=16 HEIGHT=16 BORDER=0 ALT=\"\"></A>\n";
    }
    else {
		# playlist is in error state		
    	$htmlstr .= "\t\t\<TD ALIGN=center BGCOLOR=\"#EFEFF7\">\n";
    	$htmlstr .= "\t\t\t<A HREF=\"setplsettings.cgi?cpl=$name&pg=errlog\" TARGET=\"content\">\n";
    	$htmlstr .= "\t\t\t<IMG SRC=\"../images/icon_alert.gif\" WIDTH=16 HEIGHT=16 ALT=\"\"></TD>\n";

        $htmlstr .= "\t\t<TD BGCOLOR=\"#CCCCCC\" ALIGN=CENTER><A HREF=\"startstopbcast.cgi?state=start&pl=$name\">\n";
        $htmlstr .= "\t\t\t<IMG SRC=\"../images/icon_play.gif\" WIDTH=16 HEIGHT=16 BORDER=0 ALT=\"\"></A>\n";
        $htmlstr .= "\t\t\t<IMG SRC=\"../images/icon_stop_off.gif\" WIDTH=16 HEIGHT=16 BORDER=0 ALT=\"\">\n";
    }
    $htmlstr .= "\t\t</TD>\n";

    $htmlstr .= "\t\t<TD BGCOLOR=\"#EFEFF7\" ALIGN=center>";
    $htmlstr .= "<INPUT TYPE=checkbox NAME=\"delete\" VALUE=\"$name\"></TD>\n";

    $htmlstr .= "\t</TR>\n"; 

    return $htmlstr;    
}

# -------------------------------------------------
# EmitMainPlaylistHTML(\@labels)
#
# Emit the HTML for the play lists inside $dir.
#
# returns HTML string on success or "" on failure.
# -------------------------------------------------
sub EmitMainPlaylistHTML {
    my $lblaref = $_[0];
	my $dir = &playlistlib::GetPLRootDir();
	my $state = 0;
    my $htmlstr = "";
    my $name = "";
	
	if (!(-e "$dir")) {
		# directory doesn't exist; 
		# this is probably an error.
		mkdir "$dir", 0770;
	}
	if (opendir(DIR, $dir)) {
	    while( defined ($name = readdir DIR) ) {
	    	# print all the subdirectories in $plroot.
	    	if (!(-f "$dir$name") && ($name !~ /^[.]+/)) { 
	    	    $state = &playlistlib::GetPlayListState($name);
                $htmlstr .= &playlistlib::EmitMainPlaylistRowHTML($name, $state, $lblaref);
   		    }
        }
	    closedir(DIR);
    }
    return $htmlstr;
}


# -------------------------------------------------
# SearchDirForRefMov(dirloc)
#
# Create all the files for a particular playlist 
# entry.
#
# returns a reference movie name.
# -------------------------------------------------
sub SearchDirForRefMov {
	use DirHandle;
	
    my $filedelim = &playlistlib::GetFileDelimChar();
	my $d = new DirHandle $_[0];
	my $file = '';
	my $fd;
	if (defined($d)) {
		while (defined($f = $d->read)) {
			if ($f =~ /^\./) {
				next;
			}
			else {
				if (defined($fd = new DirHandle $f)) {
					$file = SearchDirForRefMov($f);
					if ($file ne '') {
						return $_[0].$filedelim.$file;
					}
				}
				else {
					return $_[0].$filedelim.$f;
				}
			}
		}
		return '';
	}
	else {
		return '';
	}
}


# -------------------------------------------------
# SearchArrayForRefMov(\@plarrref, $movdir)
#
# Create all the files for a particular playlist 
# entry.
#
# returns a reference movie name.
# -------------------------------------------------
sub SearchArrayForRefMov {
    my $file;
	my $plr = $_[0];
	my $movdir = $_[1];
    # default to sample movie
	my $mov = $movdir;
	$mov .= "sample.mov";
	# traverse to find first actual movie
	if ($mode eq "weighted_random" ) {
		print PLDFILE "recent_movies_list_size $mr\n";
	}
	foreach $item (@$plr) {
		$file = $item;
		$file =~  /(.+)[:]([0-9]+)$/;
		$file = $1;
		if (opendir(DIR, $file)) {
			closedir(DIR);
			$file = SearchDirForRefMov($file);
			if ($file ne '') {
				return $file;
			}
		}
		else {
			return $file;
			last;
		}
	}	
	return $mov;
}


# -------------------------------------------------
# CreatePlayListEntry($plname, \@plarref, $movdir, $serverBroadcastPassword)
#
# Create all the files for a particular playlist 
# entry.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub CreatePlayListEntry {
    my $plname = $_[0];
    my $arref = $_[1];
    my $movdir = $_[2];
    my $serverBroadcastPassword = $_[3];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $uname;
    my $mode;
    my $logging;
    my $mr;
    my $plr;
    my $mov;
    my $fname;
    my $genre;
    my $destipaddr;
    my $broadcastusername;
    my $broadcastpassword;
    my $title;
    my $item;

    ($uname, $mode, $logging, $plr, $mr, $genre, $destipaddr, $broadcastusername, $broadcastpassword, $title) = @$arref;
    $plname = &playlistlib::EncodePLName($plname);
    if (!(-e $plroot)) {
    	my $newdir = $plroot;
    	$newdir =~ s/\/$//;
		mkdir "$newdir", 0770;
    }
    mkdir "$newdir", 0770;
    $fname = "$plroot$plname$filedelim$plname";
    
    if ($destipaddr eq '') {
    	$destipaddr = '127.0.0.1';
    }

    # die($mode);

    # make sure path is terminated with platform file delimeter
    if (!($movdir =~ /\/$/) && !($movdir =~ /\\$/)) {
        $movdir .= $filedelim;
    }

    # make sure $uname ends with .sdp
    if (($genre eq '') and ($uname !~ /[.][Ss][Dd][Pp]$/)) {
        $uname .= ".sdp";
    }

    if (&playlistlib::CreateNewPlaylistDir($plname, \@$plr) != 1) {
        return 0;
    }

    # replace any escaped character in URL name with underscore
    $uname =~ s/%(..)/_/ge;
    
    if (!open(PLDFILE, "> $fname.config")) {
        # failed to create playlist description file
        return 0;
    }
    if ($fname =~ /\s/) {
    	print PLDFILE "playlist_file \"$fname.playlist\"\n";
    }
    else {
    	print PLDFILE "playlist_file $fname.playlist\n";
    }
    print PLDFILE "play_mode $mode\n";
    print PLDFILE "destination_ip_address $destipaddr\n";
    if ($genre eq '') { # not a mp3 file
    	print PLDFILE "#broadcast_name \"$title\"\n";
		if ($mode eq "weighted_random" ) {
			print PLDFILE "recent_movies_list_size $mr\n";
		}
		#$mov = SearchArrayForRefMov($plr, $movdir);
		#if ($mov eq '') {
		#	my @pl = @$plr;
		#	$mov = $pl[0];
		#	$mov =~ m/(.+)[:]([0-9]+)$/;
		#	$mov = $1;
		#}
		#print PLDFILE "sdp_reference_movie \"$mov\"\n";
     	print PLDFILE "sdp_file \"$fname.sdp\"\n";
    	print PLDFILE "destination_sdp_file \"$uname\"\n";
    	print PLDFILE "broadcast_SDP_is_dynamic enabled\n";
    	if ($logging eq 'enabled') {
    		print PLDFILE "logging enabled\n";
    		print PLDFILE "log_file $fname.log\n";
    	}
    	if (($destipaddr ne '127.0.0.1') and ($destipaddr ne '')) {
    		print PLDFILE "broadcaster_name \"$broadcastusername\"\n";
    		print PLDFILE "broadcaster_password \"$broadcastpassword\"\n";
    	}
   }
    else { # mp3 file
       if ($mode eq "weighted_random" ) {
        	print PLDFILE "recent_songs_list_size $mr\n";
       }
    	print PLDFILE "destination_base_port 554\n";
    	print PLDFILE "broadcast_mount_point \"$uname\"\n";
    	print PLDFILE "broadcast_name \"$title\"\n";
    	# temporary
    	print PLDFILE "broadcast_sample_rate -1\n";
    	print PLDFILE "broadcast_genre $genre\n";
        print PLDFILE "working_dir \"$fname\"\n";
   		if ($logging eq 'enabled') {
           print PLDFILE "logging enabled\n";
    	}
    	if ($destipaddr ne '127.0.0.1') {
           print PLDFILE "broadcast_password \"$broadcastpassword\"\n";
    	}
    	else {
           print PLDFILE "broadcast_password \"$serverBroadcastPassword\"\n";
    	}
   	}
    print PLDFILE "pid_file \"$fname.pid\"\n";
    close(PLDFILE);
    chmod(0640, "$fname.config");

    return 1;
}

# -------------------------------------------------
# ParsePlayListEntry(name, $messHash)
#
# Parse the playlist entry name and return it's
# contents as an array ref.
# -------------------------------------------------
sub ParsePlayListEntry {
    my $name = $_[0];
    my $messHash = $_[1];
	my %messages = %$messHash;
    $name = &playlistlib::EncodePLName($name);
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my @datarray = ("none", "weighted_random", "disabled", "", "1", "", "127.0.0.1", "", "", $name);
    my $arg1;
    my $arg2;

    if (!open(PLDFILE, "< $plroot$name$filedelim$name.config")) {
        # failed to open playlist description file
        return \@datarray;
    }
    
    # start with empty movie list
    # and add entries as we parse them.
    while(<PLDFILE>) {
        $arg1 = "";
        $arg2 = "";
        chomp;               # no newline
        #s/#.*//;             # (we want comments now -- playlist name is in a comment)
        s/\s+$//;            # no trailing whitespace
        next unless length;  # anything left?
        if (/^([^ ]+)[ \t]+(.+)$/) {
            $arg1 = $1;
            $arg2 = $2;
        }
        # remove any double quotes surrounding the second arg
    	$arg2 =~ s/\"//g;
        if (($arg1 =~ /sdp_file/) or ($arg1 =~ /broadcast_mount_point/))
        {
        	$datarray[0] = $arg2;
        }
        elsif ($arg1 =~ /play_mode/)
        {
        	$datarray[1] = $arg2;
        }
        elsif ($arg1 =~ /logging/)
        {
        	$datarray[2] = $arg2;
        }
        elsif (($arg1 =~ /recent_movies_list_size/) or ($arg1 =~ /recent_songs_list_size/))
        {
        	$datarray[4] = $arg2;
        }
        elsif ($arg1 =~ /broadcast_genre/) 
        {
        	$datarray[5] = $arg2;
        }
        elsif ($arg1 =~ /destination_ip_address/) {
        	$datarray[6] = $arg2;
        }
        elsif ($arg1 =~ /broadcaster_name/)
        {
        	$datarray[7] = $arg2;
        }
        elsif ($arg1 =~ /broadcaster_password/)
        {
        	$datarray[8] = $arg2;
        }
        elsif ($arg1 =~ /broadcast_password/)
        {
        	$datarray[8] = $arg2;
        }
        elsif (($arg1 =~ /broadcast_name/) or ($arg1 =~ /#broadcast_name/))
        {
        	$datarray[9] = $arg2;
        }
    }
    close(PLDFILE);
    $datarray[3] = &playlistlib::ParsePlaylistFile("$plroot$name$filedelim$name.playlist");

    return \@datarray;
}

# CheckIfPidExists
# returns 0 - stopped, 1 - running
# on all platforms, if pid file does not exist => return 0
# on all platforms, if pid file exists and has value 0 => return 1
# on win32, if pid file exists with a number != 0		 => return 1
# on other platforms, if pid file exists with a number != 0
#   check for pid validity and if process exists => return 1
#						else if process doesn't exist => return 0 
sub CheckIfPidExists
{
    my $pidfile = $_[0];
    my $pid = 0;
    my $processExists = 0;
    my $pidline = "";

    if (open(PIDFILE, "< $pidfile"))
    {
		$pid = <PIDFILE>;
    	chop $pid;
    	close(PIDFILE);

	    if ($^O eq "MSWin32")
		{
			eval "require Win32::Process";
			if (defined(&Win32::Process::Open)) {
				my $obj;
				$processExists = &Win32::Process::Open($obj, $pid, 0);
				if ($processExists > 0) {
					$processExists = 1;
				}
			}
			else {
				$processExists = 1;
			}
		}
		else
		{
			if ($^O eq "solaris") { $psOutput = `ps -o comm -p $pid`; }
			else { $psOutput = `ps -o command -p $pid`; }
			
			if ($psOutput =~ /PlaylistBroadcaster/s)
			{
				$processExists = 1;
			}
			elsif ($psOutput =~ /MP3Broadcaster/s)
			{
			    $processExists = 1;
			}
			else
			{
			    $processExists = 0;
			    unlink("$pidfile");
			}
		}
    }

    return $processExists;
}

# -------------------------------------------------
# GetPlayListState(name)
#
# Returns the state of a play list.
#
# returns 0 --> stopped
#         1 --> playing
#         2 --> stopped with Errors
#         3 --> playing with Warnings 
# -------------------------------------------------
sub GetPlayListState {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    $name = &playlistlib::EncodePLName($name);
    my $targ = "$plroot$name$filedelim$name";
    my $result = 0;
	
	$processExists = CheckIfPidExists("$targ.pid");
	
	# If error file is not present
    if (!open(ERRFILE, "< $targ.err")) {	
    	# since error file doesn't exist, warnings and errors cannot be reported
    	# always returns 0 (stopped) or 1 (playing)
    	return $processExists;
    }
    
    # If error file exists look for the following
    # Errors: XX				=> errors during preflight
    # Warnings: XX				=> warnings during preflight
    # Broadcast Errors: XX		=> errors during broadcast
    # Broadcast Warnings: XX	=> warnings during broadcast
    
    $numPreflightErrors = 0; # in case there is no errors line
    $numPreflightWarnings = 0; # in case there is no warnings line
    $numBroadcastErrors = 0;
    $numBroadcastWarnings = 0;
    while ($line = <ERRFILE>) {
        # $flag = 1 indicates we have no playlist problems
        if ($line =~ /^Errors: (\d+)/) {
        	$numPreflightErrors = $1;
        }
        elsif($line =~ /^Warnings: (\d+)/) {
        	$numPreflightWarnings = $1;
        }
        elsif($line =~ /^Broadcast Errors: (\d+)/) {
        	$numBroadcastErrors = $1;
        }
        elsif($line =~ /^Broadcast Warnings: (\d+)/) {
        	$numBroadcastWarnings = $1;
        }
    }
	
	close(ERRFILE);
	
	# If process is not running
	if ($processExists == 0)
	{
		# we don't care about warnings here, only errors
		if ( ($numPreflightErrors == 0) && ($numBroadcastErrors == 0) )
		{
			$result = 0; # if there are no preflight or broadcast errors
		}
		else
		{
			$result = 2; # there are errors
		}
	}
	else	# process is running
	{
		#obviously there shouldn't have been any errors, so just check for warnings
		if ( ($numPreflightWarnings == 0) && ($numBroadcastWarnings == 0) )
		{
			$result = 1; # if there are no preflight or broadcast warnings
		}
		else
		{
			$result = 3; # there are warnings
		}
	}

    return $result;
}

# -------------------------------------------------
# LaunchWin32Process (command, name, args, [0|1])
# input:	command = path to the binary
# 			name = program name
# 			args = arguments to the program
# 			[0|1] = 0 if you want to return right away
#				 and 1 if you want to wait until the process exits
# returns: 1 => success
#		   0 => failure
# -------------------------------------------------
sub LaunchWin32Process
{
	my $command = $_[0];
	my $name = $_[1];
	my $args = $_[2];
	my $wait = $_[3];
	my $processObj;
	my $exitCode = 0;
	
	eval "require Win32::Process";
	
	if (!$@)
	{
    	Win32::Process::Create($processObj,"$command","$name $args",1,DETACHED_PROCESS,".");
    	$processObj->SetPriorityClass(NORMAL_PRIORITY_CLASS);
    	
    	if ($wait == 0)
    	{
    		$processObj->Wait(0); # don't wait for process to complete
    	}
    	else
    	{
    		$processObj->Wait(INFINITE); # wait until the process completes
    	}
    	
    	$processObj->GetExitCode($exitCode);
    	
    	#if ($exitCode != 0)
    	#{
    	#	return 0;
    	#}
    }
    else
    {
    	return 0;
    }

	return 1;
}

# -------------------------------------------------
# PreflightPlayList(name, isMP3)
#
# Preflight a playlist and return the results of the preflight
# returns:	0 => no errors and no warnings
# 			1 => warnings but no errors
# 			2 => errors
# -------------------------------------------------
sub PreflightPlayList
{
    my $name = $_[0];
    my $isMP3 = $_[1];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $targ = "$plroot$name$filedelim$name";
	my $program = "";
	my $broadcaster = "";
	my $args = "";
	
	if ($isMP3 == 1)
	{
    	$program = $ENV{"QTSSADMINSERVER_QTSSMP3BROADCASTER"};
    	$broadcaster = "MP3Broadcaster";
    	$args = "-x -e \"$targ.err\" -c";
    }
    else
    {
    	$program = $ENV{"QTSSADMINSERVER_QTSSPLAYLISTBROADCASTER"};
    	$broadcaster = "PlaylistBroadcaster";
    	$args = "-p -f -d -e \"$targ.err\"";
	}
    
    # preflight the playlist
    if ($^O eq "MSWin32")
    {
    	$result = &playlistlib::LaunchWin32Process($program, $broadcaster, "$args \"$targ.config\"", 1);
    	if ($result == 0) # couldn't launch the process!
    	{
    		return 2;
    	}
    }
    else
    {
    	system "$program $args \"$targ.config\"";
    }
    
    
    # return the preflight status
    return &playlistlib::PreflightStatus("$targ.err");
}

# -------------------------------------------------
# GetPreflightStatus ( errorFile )
# input: error filename
# returns:	0 => no errors and no warnings
# 			1 => warnings but no errors
# 			2 => errors
# -------------------------------------------------
sub PreflightStatus
{
	my $errorFile = $_[0];
	my $errorLineFound = 0;
    my $count = 0; 
    my $numErrors = 1; # assume that there are errors if no errors line is found
    my $numWarnings = 0;
    my $line = "";
    
    while ( ($errorLineFound == 0) && ($count < 5) )
    {
    	if (open(ERRFILE, "< $errorFile"))
    	{
    		while ($line = <ERRFILE>)
    		{
	        	if ($line =~ /Errors: (\d+)/) {
    	    		$numErrors = $1;
    	    		$errorLineFound = 1;
    	    	}
    	    	elsif($line =~ /Warnings: (\d+)/) {
    	    		$numWarnings = $1;
        		}
    		}
    		
    		close(ERRFILE);
    	}
    	
    	if ($errorLineFound == 1)
    	{
    		last;
    	}
    	
    	$count++;
    	sleep(2);
    }
    
    if ($numErrors != 0)
    {
    	return 2;
    }
    elsif ($numWarnings != 0)
    {
    	return 1;
    }
    	
    return 0;
}

# -------------------------------------------------
# StartPlayList(name, isMP3)
# input:	name = playlist name
#			isMP3 = 1 for MP3, 0 for PLB
# returns:	1 => success
#			0 => failure
# -------------------------------------------------
sub StartPlayList
{
    my $name = $_[0];
    my $isMP3 = $_[1];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    $name = &playlistlib::EncodePLName($name);
    my $targ = "$plroot$name$filedelim$name";
    
    my $processObj;
    my $program = "";
    my $broadcaster = "";
    my $args = "";
    my $preflightResult = 0;
    
    if ($isMP3 == 1)
    {
    	$program = $ENV{"QTSSADMINSERVER_QTSSMP3BROADCASTER"};
    	$broadcaster = "MP3Broadcaster";
    	$args = "-e \"$targ.err\" -c";
    }
    else
	{
		$program = $ENV{"QTSSADMINSERVER_QTSSPLAYLISTBROADCASTER"};
		$broadcaster = "PlaylistBroadcaster";
		$args = "-a -f -e \"$targ.err\"";
		
	}
	  
	# if pid file already exists => playlist already running, so return
    if (CheckIfPidExists("$targ.pid") == 1) { return 0; }

	# remove the previous local SDP file if it exists  
    if (-e "$targ.sdp") { unlink "$targ.sdp"; }
	
	# delete the error file before starting the preflight
	if (-e "$targ.err") { unlink "$targ.err"; }
    
    $preflightResult = &playlistlib::PreflightPlayList($name, $isMP3);

    if ($preflightResult == 2) { return 0; }
     

    
    if ($^O eq "MSWin32")
    {
    	$result = &playlistlib::LaunchWin32Process($program, $broadcaster, "$args \"$targ.config\"", 0);
    	if ($result == 0) # couldn't launch the process!
	{
    		return 2;
	}
    }
    else
	{
		system "$program $args \"$targ.config\"";
	}
	
	# wait until the pid file gets written before returning
	my $count = 0;
	my $pidFileFound = 0;
	while ($count < 5)
	{
		sleep(2);
		if (-e "$targ.pid")
		{
			$pidFileFound = 1;
			last;
       	}  	
       	$count++  	
	}
	
	# if pid file wasn't written even after waiting for 10seconds, return failure status
	if ($pidFileFound == 0)
	{
    	     return 0;
        }

    return 1;
}

# -------------------------------------------------
# StopPlayList(name)
#
# Stop the playlist broadcast by name.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub StopPlayList
{
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    $name = &playlistlib::EncodePLName($name);
    my $targ = "$plroot$name$filedelim$name";
    my $pid;

    if (!open(PIDFILE, "< $targ.pid"))
    {
        # can't open the pid file to read the PID.
	unlink "$targ.pid";
        return 0;
    }
    
    $pid = <PIDFILE>;
    chop $pid;
    close(PIDFILE);
    
    if ($^O eq "MSWin32")
    {
    	eval "require Win32::Process";
    	if (!$@ && $pid != 0)
    	{
           Win32::Process::KillProcess($pid, 0);
    	}
    }
    elsif ($pid != "" && $pid != 0)
    {
        kill 15, $pid;
    }
    else
    {
    	return 0;
    }
    
    if (-e "$targ.sdp") { unlink "$targ.sdp"; } 
    
    if (-e "$targ.pid") { unlink "$targ.pid"; }
    #if (-e "$targ.current") { unlink "$targ.current"; }
    #if (-e "$targ.upcoming") { unlink "$targ.upcoming"; }
    
    return 1;
}

1; #return true  

