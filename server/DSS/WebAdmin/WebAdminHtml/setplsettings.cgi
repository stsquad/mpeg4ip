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
if ($plroot eq  "") {
    $plroot = "/Library/QuickTimeStreaming/Playlists/";
}

$messageHash = &adminprotolib::GetMessageHash();

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

# HTTP parameter line should look like this:
#
# setplsettings.cgi?cpl=some_playlist_name&pg=detail
#          - OR -
# setplsettings.cgi?cpl=some_playlist_name&pg=errlog
#

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

# parse the CGI parameters
if ($qs =~ /cpl=(.+)&pg=(.+)$/) {
    $pln = $1;
    $pg = $2;
}
else
{
	# invalid CGI params
    $pln = "none";
    $pg = "detail";
}

$status = 500;

if($pg eq "detail") {
	# setup for emitting the detail for a given playlist
    &playlistlib::PushCurrPlayList($pln);
	$filename = "settings/settings_pl_details.htm";
}
elsif($pg eq "errlog") {
	# setup for emitting the playlist error log file.
	$filename = "none";
	&cgilib::PrintOKTextHeader($servername);
    print &playlistlib::EmitPlayListErrLogFile($pln);
}
else {
	# invalid CGI params
	$filename = "settings/settings_playlists.htm";
}

chdir($accessdir);

if(($filename ne "none") && (-e $filename) && (-r $filename)) {
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
elsif ($pg ne "errlog") {
	# invalid CGI params
    &cgilib::PrintNotFoundResponse($servername, $filename, $messageHash);
}

