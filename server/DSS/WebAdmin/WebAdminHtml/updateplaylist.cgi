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

require ("./adminprotocol-lib.pl");
require ("./cgi-lib.pl");

# Get the server name, QTSS IP address, and Port from ENV hash
$servername = $ENV{"SERVER_SOFTWARE"};
$qtssip = $ENV{"QTSSADMINSERVER_QTSSIP"};
$qtssport = $ENV{"QTSSADMINSERVER_QTSSPORT"};
$qtssname = $ENV{"QTSSADMINSERVER_QTSSNAME"};
$remoteip = $ENV{"REMOTE_HOST"};
$accessdir = $ENV{"LANGDIR"};

$hostname = $ENV{"SERVER_NAME"};
$hostport = $ENV{"SERVER_PORT"};

$messageHash = &adminprotolib::GetMessageHash();

$plroot = $ENV{"PLAYLISTS_ROOT"};
if ($plroot eq "") {
    $plroot = "/Library/QuickTimeStreaming/Playlists/";
}

$auth = $ENV{"HTTP_AUTHORIZATION"};
$authheader = "";
if(defined($auth)) {
	$authheader = "Authorization: $auth\r\n";  
}
else
{
	$data = "";
	&cgilib::PrintChallengeResponse($servername, $data, $messageHash);
	return;
}

# post method environment variable
$cl = $ENV{"CONTENT_LENGTH"};

if ($ENV{"REQUEST_METHOD"} eq "POST")
{
    # put the data into a variable
    read(STDIN, $qs, $cl);
}
else
{
    $qs = $ENV{"QUERY_STRING"};
}

# See if the server is running. (we use this to force authentication)
# A $status == 401 means we failed authentication.

$status = &adminprotolib::EchoData($data, $messageHash, $authheader, $qtssip, $qtssport, "/modules/admin/server/qtssSvrState", "/server/qtssSvrState");

# A backtick in the CGI params is illegal. We will force authentication to fail.
if ($qs =~ /[`]/) {
	$status = 401;
}

if($status == 401) {
	$data = "";
	&cgilib::PrintChallengeResponse($servername, $data, $messageHash);
	return;
}

$filename = "settings/settings_pl_details.htm";
chdir($accessdir);

if ($qs eq "") {
	&cgilib::PrintOKTextHeader($servername);
	print "Error: Empty form submitted!\r\n";
}
else
{
    #
    # update the playlist here
    #
    my @formparms = split /&/, $qs;
    my $name = "none";
    my $url = "sample.sdp";
    my $mode = "sequential";
    my $logging = "disabled";
    my $movdir;
    my $mr = 0;
    my @pla = ();
    my @plo = ();
    my @plc;
    my $rv;
    
    foreach $prm (@formparms) {
        if ($prm =~ /playlistname=(.*)$/) {
            $name = $1;
            # convert plus to underscore
            $name =~ s/[+]/_/g;
        }
        elsif ($prm =~ /playlistURL=(.*)$/) {
            $url = $1;
        }
        elsif ($prm =~ /mode=(.*)$/) {
            $mode = $1;
        }
        elsif ($prm =~ /maxrepeats=(.*)$/) {
            $mr = $1;
        }
        elsif ($prm =~ /logging=(.*)$/) {
            $logging = $1;
        }
        elsif ($prm =~ /order=(.*)$/) {
            push @plo, $1;
        }
        elsif ($prm =~ /weight=(.*)$/) {
            $movie = $1;
            # convert plus to whitespace
            $movie =~ s/[+]/ /g;
    
            # convert the hex characters back to ASCII
            $movie =~ s/%(..)/pack("c",hex($1))/ge;
        	push @pla, $movie;
        }
    }
    $rv = &playlistlib::SortPlayList(\@plo, \@pla);
    @pla = (@$rv);
    @plc = ($url, $mode, $logging, \@pla, $mr);
    &playlistlib::PushCurrPlayList($name);
    $status = &adminprotolib::GetMovieDir($movdir, $messageHash, $authheader, $qtssip, $qtssport);
    &playlistlib::CreatePlayListEntry($name, \@plc, $movdir);
}

if((-e $filename) && (-r $filename)) {
    $data = "";
    $status = &adminprotolib::ParseFile($data, $authheader, $qtssip, $qtssport, $filename, "IFREMOTE",  "ipaddress", $remoteip);
    
    if($status == 401) {
	    &cgilib::PrintChallengeResponse($servername, $data, $messageHash);
    }
    else {
	    &cgilib::PrintOKTextHeader($servername);
	    print $data;
    }
}
else {
    &cgilib::PrintNotFoundResponse($servername, $filename, $messageHash);
}

