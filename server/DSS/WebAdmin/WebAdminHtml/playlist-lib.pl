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
	if ($plroot == "") {
	    if ($^O =~/[Dd]arwin/) {
            $plroot = "/Library/QuickTimeStreaming/Playlists/";
        }
        elsif ($^O eq "MSWin32") {
            $plroot = "C:\\Program Files\\Darwin Streaming Server\\Playlists\\";
        }
        else {
            $plroot = "/var/streaming/playlists/";
        }
    }
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
    my $htmlstr = "<html><body><tt>\n";
    
    if (!open(ERRFILE, "< $plroot$name$filedelim$name.err")) {
        # failed to open error log file
    	$htmlstr .= "&nbsp;Log not found!<br></tt></body></html>\n";
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
    $htmlstr .= "</tt></body></html>\n";
    
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
        if ($movie =~ /\s/) {
            print PLFILE "\"$movie\" $wt\n";
        }
        else {
            print PLFILE "$movie $wt\n";
        }
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
        s/#.*//;             # no comments
        s/\s+$//;            # no trailing whitespace
        next unless length;  # anything left?
        if (/^["]([^"]+)["][ \t]*([0-9]*)$/) {
            $arg1 = $1;
            $arg2 = $2;
        }
        elsif (/^([^ ]+)[ \t]*([0-9]*)/) {
            $arg1 = $1;
            $arg2 = $2;
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
	    	if ($file !~ /^[.]+/) {
	    	    $file = "$dir$filedelim$file";
			    if (!(unlink($file))) {
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
# CreatePlayListEntry($plname, \@plarref, $movdir)
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
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $uname;
    my $mode;
    my $logging;
    my $mr;
    my $plr;
    my $mov;
    my $fname;
    
    ($uname, $mode, $logging, $plr, $mr) = @$arref;
    $plname = &playlistlib::EncodePLName($plname);
    $fname = "$plroot$plname$filedelim$plname";
    
    # make sure path is terminated with platform file delimeter
    if (!($movdir =~ /\/$/) && !($movdir =~ /\\$/)) {
        $movdir .= $filedelim;
    }
   
    # make sure $uname ends with .sdp
    if ($uname !~ /[.][Ss][Dd][Pp]$/) {
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
    if ($mode eq "weighted_random" ) {
    	print PLDFILE "recent_movies_list_size $mr\n";
    }
    print PLDFILE "destination_ip_address 127.0.0.1\n";
    print PLDFILE "destination_base_port 0\n";
    # The first movie in the playlist is
    # assumed to be the reference movie
    if ( @$plr > 0) {
    	($mov) = @$plr;
    	$mov =~ /(.+)[:]([0-9]+)$/;
    	$mov = $1;
        if ($mov =~ /\s/) {
    	    print PLDFILE "sdp_reference_movie \"$mov\"\n";
    	}
    	else {
    	    print PLDFILE "sdp_reference_movie $mov\n";
    	}
    } 
    else {
        $mov = $movdir;
        $mov .= "sample.mov";
    	print PLDFILE "sdp_reference_movie $mov\n";
    }
    print PLDFILE "logging $logging\n";
    print PLDFILE "sdp_file $uname\n";
    print PLDFILE "destination_sdp_file $uname\n";
    print PLDFILE "broadcast_SDP_is_dynamic enabled\n";
    close(PLDFILE);
    
    return 1;
}

# -------------------------------------------------
# ParsePlayListEntry(name)
#
# Parse the playlist entry name and return it's
# contents as an array ref.
# -------------------------------------------------
sub ParsePlayListEntry {
    my $name = $_[0];
    $name = &playlistlib::EncodePLName($name);
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my @datarray = ("none", "weighted_random", "disabled", "", "1");
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
        s/#.*//;             # no comments
        s/\s+$//;            # no trailing whitespace
        next unless length;  # anything left?
        if (/^(.+)[ \t]+(.+)$/) {
            $arg1 = $1;
            $arg2 = $2;
        }
        # remove any double quotes surrounding the second arg
    	$arg2 =~ s/\"//g;
        if ($arg1 =~ /sdp_file/)
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
        elsif ($arg1 =~ /recent_movies_list_size/)
        {
        	$datarray[4] = $arg2;
        }
    }
    close(PLDFILE);
    $datarray[3] = &playlistlib::ParsePlaylistFile("$plroot$name$filedelim$name.playlist");
    
    return \@datarray;
}

# -------------------------------------------------
# GetPlayListState(name)
#
# Returns the state of a play list.
#
# returns 1 for playing or 0 for stopped
# -------------------------------------------------
sub GetPlayListState {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    $name = &playlistlib::EncodePLName($name);
    my $targ = "$plroot$name$filedelim$name";
	
    if (-e "$targ.err") {
        # an error in the playlist exists
        return 2;
    }
    if (-e "$targ.pid") {
        # the playlist is running from UI launch
        return 1;
    }
    if (-e "$targ.current") {
        # the playlist is running from local command line launch
        return 1;
    }
    else {
        # the playlist is not running
	    return 0;
	}
}

# -------------------------------------------------
# PreflightPlayListWin32(name)
#
# Preflight a playlist to check for errors on Win32.
#
# returns 1 on success or 0 on error discovery.
# -------------------------------------------------
sub PreflightPlayListWin32 {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $targ = "$plroot$name$filedelim$name";
    my $line;
    my $flag = 0;
    my $exitCode = 0;
    
    # remove the previous local SDP file if it exists  
    if (-e "$targ.sdp") {
        unlink "$targ.sdp";
    }   
    # remove the previous local error file if it exists  
    if (-e "$targ.err") {
        unlink "$targ.err";
    }
    #
    # preflight the playlist piping the error to a local error file 
    #  
    eval "require Win32::Process";
    if (!$@) {
    	Win32::Process::Create(
    		$processObj,
    		"c:\\Program Files\\Darwin Streaming Server\\PlaylistBroadcaster.exe",
    		"PlaylistBroadcaster \"$targ.config\" -apf",
    		1,
    		DETACHED_PROCESS,
    		".");
    	$processObj->SetPriorityClass(NORMAL_PRIORITY_CLASS);
    	# wait until the process completes.
    	$processObj->Wait(INFINITE);
    	$processObj->GetExitCode($exitCode);
    	if ($exitCode != 0) {
            if (open(ERRFILE, "> $targ.err")) {
                print ERRFILE "\nInvalid playlist config file or invalid playlist!\n";
                close(ERRFILE);
			}
    	}
   }
    else {
    	return 0;
    }
    #
    # parse the local error file looking for a no error condition
    #
    if (!open(ERRFILE, "< $targ.err")) {
        # can't open a file that holds the error status.
        return 0;
    }
    while ($line = <ERRFILE>) {
        # $flag = 1 indicates we have no playlist problems
        if ($line =~ /found \(0\) movie problem/) {
        	$flag = 1;
        }
    }
    close(ERRFILE);
    #
    # cleanup error file if not needed
    #
    if ($flag == 1) {
        # no errors so delete the error file
        unlink "$targ.err";
    }
    else {
        # assume we must have discovered an error
    	return 0;
    }
    return 1;
}

# -------------------------------------------------
# PreflightPlayList(name)
#
# Preflight a playlist to check for errors.
#
# returns 1 on success or 0 on error discovery.
# -------------------------------------------------
sub PreflightPlayList {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    my $targ = "$plroot$name$filedelim$name";
    my $line;
    my $flag = 0;
    my $plba = "/usr/local/bin/PlaylistBroadcaster";

    # remove the previous local SDP file if it exists  
    if (-e "$targ.sdp") {
        unlink "$targ.sdp";
    }   
    # remove the previous local error file if it exists  
    if (-e "$targ.err") {
        unlink "$targ.err";
    }
    #
    # preflight the playlist piping the error to a local error file 
    #  
    system "$plba \"$targ.config\" -apf > \"$targ.err\"";
    #
    # parse the local error file looking for a no error condition
    #
    if (!open(ERRFILE, "< $targ.err")) {
        # can't open a file that holds the error status.
        return 0;
    }
    while ($line = <ERRFILE>) {
        # $flag = 1 indicates we have no playlist problems
        if ($line =~ /found \(0\) movie problem/) {
        	$flag = 1;
        }
    }
    close(ERRFILE);
    #
    # cleanup error file if not needed
    #
    if ($flag == 1) {
        # no errors so delete the error file
        unlink "$targ.err";
    }
    else {
        # assume we must have discovered an error
    	return 0;
    }
    return 1;
}

# -------------------------------------------------
# StartPlayListWin32(name)
#
# Start the playlist broadcast by name for Win32.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub StartPlayListWin32 {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    $name = &playlistlib::EncodePLName($name);
    my $targ = "$plroot$name$filedelim$name";
    my $processObj;
    
    if (-e "$targ.pid") {
        return 0;
    }
    #
    # Preflighting doesn't work om Win32 under our current
    # scheme. We need to plug in a solution that works here
    # at some future time.
    #  
    #&playlistlib::PreflightPlayListWin32($name);
    #if (-e "$targ.err") {
    #    return 0;
    #}   
   	if (-e "$targ.sdp") {
        unlink "$targ.sdp";
    }   
    eval "require Win32::Process";
    if (!$@) {
    	Win32::Process::Create(
    		$processObj,
    		"c:\\Program Files\\Darwin Streaming Server\\PlaylistBroadcaster.exe",
    		"PlaylistBroadcaster \"$targ.config\" -af",
    		1,
    		DETACHED_PROCESS,
    		".");
    	$processObj->SetPriorityClass(NORMAL_PRIORITY_CLASS);
    	$processObj->Wait(0);
        # get child's PID
    	$pid = $processObj->GetProcessID();
        # parent process records child's PID
    	if (!open(PIDFILE, "> $targ.pid")) {
        	# can't open a file to hold the PID.
        	return 0;
    	}
    	print PIDFILE "$pid\n";
    	close(PIDFILE);
    }
    else {
    	return 0;
    }
    return 1;
}

# -------------------------------------------------
# StartPlayList(name)
#
# Start the playlist broadcast by name.
#
# returns 1 on success or 0 on failure.
# -------------------------------------------------
sub StartPlayList {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    $name = &playlistlib::EncodePLName($name);
    my $targ = "$plroot$name$filedelim$name";
    
    if (-e "$targ.pid") {
        return 0;
    }   
    &playlistlib::PreflightPlayList($name);
    if (-e "$targ.err") {
        return 0;
    }   
   if (-e "$targ.sdp") {
        unlink "$targ.sdp";
    }   
    if ($pid = fork) {
        # parent process records child's PID
    	if (!open(PIDFILE, "> $targ.pid")) {
        	# can't open a file to hold the PID.
        	return 0;
    	}
    	print PIDFILE "$pid\n";
    	close(PIDFILE);
    }
    else {
        # child process launches playlist braodcaster and dies
    	exec "/usr/local/bin/PlaylistBroadcaster \"$targ.config\" -adf";
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
sub StopPlayList {
    my $name = $_[0];
    my $plroot = &playlistlib::GetPLRootDir();
    my $filedelim = &playlistlib::GetFileDelimChar();
    $name = &playlistlib::EncodePLName($name);
    my $targ = "$plroot$name$filedelim$name";
    my $pid;
    
    if (-e "$targ.current") {
        unlink "$targ.current";
    }
    if (-e "$targ.sdp") {
        unlink "$targ.sdp";
    }   
    if (!open(PIDFILE, "< $targ.pid")) {
        # can't open a file to hold the PID.
        return 0;
    }
    $pid = <PIDFILE>;
    chop $pid;
    close(PIDFILE);
    unlink "$targ.pid";
    if ($^O eq "MSWin32") {
    	eval "require Win32::Process";
    	if (!$@ && $pid != 0) {
    		Win32::Process::KillProcess($pid, 0);
    	}
    }
    elsif ($pid != "" && $pid != 0) {
        kill 15, $pid;
    }
    else {
    	return 0;
    }
    if (-e "$targ.upcoming") {
        unlink "$targ.upcoming";
    }
    return 1;
}

1; #return true  

