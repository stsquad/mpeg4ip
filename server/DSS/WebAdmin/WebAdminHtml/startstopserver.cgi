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

$hostname = $ENV{"SERVER_NAME"};
$hostport = $ENV{"SERVER_PORT"};
$redirectpage = "parsewithinput.cgi?filename=nav.htm&stateChange=1";

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

	$qsRef = &adminprotolib::ParseQueryString($qs);
  	%qs =  %$qsRef;
 }
 elsif ($ENV{"REQUEST_METHOD"} eq "GET")
 {
 	$qs = $ENV{"QUERY_STRING"};
 	$qsRef = &adminprotolib::ParseQueryString($qs);
  	%qs =  %$qsRef;
 }

$setting = $qs{"StopServer"};
$data = "";
$status = &adminprotolib::EchoData($data, $messageHash, $authheader, $qtssip, $qtssport, "/modules/admin/server/qtssSvrState", "/server/qtssSvrState");
# if the server is not running, launch process
if($status == 500) {
	&adminprotolib::StartServer($qtssname);
	sleep(4);
	$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, "/admin/server/qtssSvrState", 1);	        
}
elsif($status == 200) {
	# if server is running, then put it in idle mode or if server is idle/not running, then put it in running mode
	if($data == 1) {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, "/admin/server/qtssSvrState", 5);
	}
	else {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, "/admin/server/qtssSvrState", 1);
	}
}

if($status != 401) {
	&cgilib::PrintRedirectHeader($servername, $hostname, $hostport, $redirectpage);
}
elsif($status == 401) {
	&cgilib::PrintChallengeResponse($servername, $data, $messageHash);
}


