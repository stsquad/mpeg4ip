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
$accessdir = $ENV{"LANGDIR"};

$auth = $ENV{"HTTP_AUTHORIZATION"};
$authheader = "";
if(defined($auth)) {
	$authheader = "Authorization: $auth\r\n";  
}

$messageHash = &adminprotolib::GetMessageHash();

# post method environment variable
$cl = $ENV{"CONTENT_LENGTH"};

if ($ENV{"REQUEST_METHOD"} eq "POST")
 {
  	# put the data into a variable
  	read(STDIN, $qs, $cl);
	$qsRef = &adminprotolib::ParseQueryString($qs);
  	%qs =  %$qsRef;
 }
 elsif ($ENV{"REQUEST_METHOD"} eq "GET")
 {
 	$qs = $ENV{"QUERY_STRING"};
 	$qsRef = &adminprotolib::ParseQueryString($qs);
  	%qs =  %$qsRef;
 }
  	
$filename = $qs{"filename"};
$numEntries = $qs{"numEntries"};
$refreshInterval = $qs{"refreshInterval"};
$order = $qs{"sortOrder"};
@numEntriesArr = ("20", "50", "100", "200", "-1");
$numIndex = $numEntriesArr[$numEntries];
$serverStateChanged = $qs{"stateChange"};

$data = "";

chdir($accessdir);
if($^O ne "MSWin32") {
	chroot($accessdir);
}

if(($^O eq "MSWin32") && &adminprotolib::CheckIfForbidden($accessdir, $filename)) {
    &cgilib::PrintNotFoundResponse($servername, $filename, $messageHash);
}
elsif((-e $filename) && (-r $filename)) {
	if($filename eq "nav.htm") {
		$status = &adminprotolib::ParseFile($data, $authheader, $qtssip, $qtssport, $filename, "GENJAVASCRIPTIFSTATECHANGE", "stateChange", $serverStateChanged);	
	}
	else {
		$status = &adminprotolib::ParseFile($data, $authheader, $qtssip, $qtssport, $filename, 
					"CREATESELECTFROMINPUTWITHFUNC", "numEntries", $numEntries,
					"CREATESELECTFROMINPUTWITHFUNC", "refreshInterval", $refreshInterval,
					"CREATESELECTFROMINPUTWITHFUNC", "sortOrder", $order, 
					"SORTRECORDSWITHINPUT", "sortOrder", $order,
					"FORMATDDARRAYWITHINPUT", "numEntries", $numIndex);
	}
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
