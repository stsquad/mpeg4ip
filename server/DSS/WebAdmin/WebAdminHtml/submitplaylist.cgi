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

$plroot = $ENV{"PLAYLISTS_ROOT"};
if ($plroot eq "") {
    $plroot = "/Library/QuickTimeStreaming/Playlists/";
}

$messageHash = &adminprotolib::GetMessageHash();

$auth = $ENV{"HTTP_AUTHORIZATION"};
$authheader = "";
if(defined($auth)) {
	$authheader = "Authorization: $auth\r\n";  
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

$filename = "settings/settings_pl_details.htm";
chdir($accessdir);

if ($qs =~ /Create/)
{
    #
    # This just takes us to our create playlist page
    #
    $filename = "settings/settings_pl_create.htm";
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
elsif ($qs =~ /Delete/)
{
    #
    # This will delete all the checked playlist directories by name
    #
    my @formparms = split /&/, $qs;
    my $plname;
    my $dname;
    
    foreach $prm (@formparms) {
        if ($prm =~ /delete=(.*)$/) {
        	$plname = $1;
            $plname =~ s/%(..)/pack("c",hex($1))/ge;
            
        	&playlistlib::RemovePlaylistDir($plname);
        }
    }
    $filename = "settings/settings_playlists.htm";
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
    #
    # Some unknown form type was submitted.
    # (should never happen if the html is valid...)
    #
    &cgilib::PrintNotFoundResponse($servername, $filename, $messageHash);
}
