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
$remoteip = $ENV{"REMOTE_HOST"};
$accessdir = $ENV{"LANGDIR"};

$auth = $ENV{"HTTP_AUTHORIZATION"};
$authheader = "";
if(defined($auth)) {
	$authheader = "Authorization: $auth\r\n";  
}

#Get the filename from argv
$filename = $ARGV[0];

$messageHash = &adminprotolib::GetMessageHash();

chdir($accessdir);

if($^O ne "MSWin32") {
	if(!(($filename eq "logs/logs_error.htm")  || 
         ($filename eq "logs/logs_access.htm") ||
	     ($filename eq "settings/settings_general.htm") ||
         ($filename eq "settings/settings_pl_create.htm") ||
         ($filename =~ /settings\/settings_pl_edit/) ||
         ($filename eq "settings/settings_playlists.htm"))) {
		chroot($accessdir);
	}
}

if(($^O eq "MSWin32") && &adminprotolib::CheckIfForbidden($accessdir, $filename)) {
    &cgilib::PrintNotFoundResponse($servername, $filename, $messageHash);
}
elsif((-e $filename) && (-r $filename)) {
    $data = "";
    $status = &adminprotolib::ParseFile($data, $authheader, $qtssip, $qtssport, $filename, "IFREMOTE",  "ipaddress", $remoteip);
    
    if($status == 401) {
	    &cgilib::PrintChallengeResponse($servername, $data, $messageHash);
    }
    elsif(($filename ne "nav.htm") && ($status eq "500")) {
	    &cgilib::PrintOKTextHeader($servername);
	    &cgilib::PrintServerNotRunningHtml($messageHash);
    }
    else {
	    &cgilib::PrintOKTextHeader($servername);
	    print $data;
    }
}
else {
    &cgilib::PrintNotFoundResponse($servername, $filename, $messageHash);
}


