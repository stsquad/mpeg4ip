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

$hostname = $ENV{"SERVER_NAME"};
$hostport = $ENV{"SERVER_PORT"};
$redirectpage = "parse.cgi?settings/settings_logging.htm";

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

for $setting (keys %qs) {          
	$errorPath = "/admin/server/qtssSvrPreferences/";
	$accessPath = "/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/";
	$attrValue = $qs{$setting};
	
	if($setting eq "Error Log Roll Time") {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $errorPath."error_logfile_interval", $attrValue);		
	}
	elsif($setting eq "Error Log Roll Size") {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $errorPath."error_logfile_size", ($attrValue * 1024));
	}
	elsif($setting eq "Access Log Roll Time") {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $accessPath."request_logfile_interval", $attrValue);
	}
	elsif($setting eq "Access Log Roll Size") {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $accessPath."request_logfile_size", ($attrValue * 1024));
	}
	elsif($setting eq "Log Errors") {
		if($attrValue eq "true") {
			$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $errorPath."error_logging", true);				
		}
		elsif($attrValue eq "false") {
			$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $errorPath."error_logging", false);
		}
	}
	elsif($setting eq "Log Access") {
		if($attrValue eq "true") {
			$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $accessPath."request_logging", true);				
		}
		elsif($attrValue eq "false") {
			$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $accessPath."request_logging", false);
		}
	}
	if($status == 401) {
		last;
	}
}

if($status != 401) {
	&cgilib::PrintRedirectHeader($servername, $hostname, $hostport, $redirectpage);
}
elsif($status == 401) {
	&cgilib::PrintChallengeResponse($servername, $data, $messageHash);
}

