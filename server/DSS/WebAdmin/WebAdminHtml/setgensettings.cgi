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
$configPath = $ENV{"QTSSADMINSERVER_CONFIG"};

$qtssPasswdName = $ENV{"QTSSADMINSERVER_QTSSQTPASSWD"};
$qtssAdmin = $ENV{"QTSSADMINSERVER_QTSSADMIN"};

$hostname = $ENV{"SERVER_NAME"};
$hostport = $ENV{"SERVER_PORT"};
$redirectpage = "parse.cgi?settings/settings_general.htm";

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

$usersFileAttributePath = "/server/qtssSvrModuleObjects/QTSSAccessModule/qtssModPrefs/modAccess_usersfilepath";

for $setting (keys %qs) {          
	$data = "";
	$settingsPath = "/admin/server/qtssSvrPreferences/";
	$attrValue = $qs{$setting};
	
	if($setting eq "Movies Directory") {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $settingsPath."movie_folder", $attrValue);		
	}
	elsif($setting eq "Authentication Scheme") {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $settingsPath."authentication_scheme", $attrValue);
	}
	elsif($setting eq "Maximum Number of Connections") {
		$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $settingsPath."maximum_connections", $attrValue);
	}
	elsif($setting eq "Streaming on Port 80") {
		if($attrValue eq "on") {
			$status = &adminprotolib::AddValueToAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $settingsPath."rtsp_port", 80);				
		}
		elsif($attrValue eq "off") {
			$status = &adminprotolib::DeleteValueFromAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $settingsPath."rtsp_port", 80);
		}
	}
	elsif($setting eq "Auto Start") {
		# if file doesn't exist, then create it and add the autostart pref to it
		$configLine = qq(qtssAutoStart=$attrValue\n);
		if(!(-e $configPath)) {
			open(CONFIGFILE, ">$configPath");
			print CONFIGFILE $configLine;
			close(CONFIGFILE);
			$ENV{"QTSSADMINSERVER_QTSSAUTOSTART"} = $attrValue;
		}
		else {
			if( -r $configPath && -w _) {
				$tmpname = $configPath . ".tmp";
				open(TEMPFILE, ">$tmpname");
				open(CONFIGFILE, "<$configPath");
				while($line = <CONFIGFILE>) {
	    			$_ = $line;
	    			chop;
	    			if (/^#/ || !/\S/) {
	    				print TEMPFILE $line;
						next; 
	    			}
	    			/^([^=]+)=(.*)$/;
	    			$name = $1; $val = $2;
	    			$name =~ s/^\s+//g; $name =~ s/\s+$//g;
	    			$val =~ s/^\s+//g; $val =~ s/\s+$//g;
					if($name eq "qtssAutoStart") {
						$val = $attrValue; 
					}
					$config{$name} = $val;
					print TEMPFILE qq($name=$val\n);
				}
				if(!defined($config{'qtssAutoStart'})) {
					print TEMPFILE $configLine;
				}
				close(CONFIGFILE);
				close(TEMPFILE);
				rename $tmpname, $configPath;
				$ENV{"QTSSADMINSERVER_QTSSAUTOSTART"} = $attrValue;
			}
			else {
				print STDERR "couldn't open $configPath for writing\n";
			}
		}
	}
	elsif($setting eq "Administrator Password") {
		$status = &adminprotolib::SetPassword($data, $messageHash, $authheader, $qtssip, $qtssport, $usersFileAttributePath, $attrValue, $qtssPasswdName, $qtssAdmin);
	}
	if($status == 401) {
		last;
	}
}

# If either the value or the units or both changed, set the value
if(($status != 401) && ($qs{"Maximum Throughput"} || $qs{"Display Throughput"})) {
	$bw = defined(($val = $qs{"Maximum Throughput"}))? $val : $qs{"Maximum Throughput\$"};
	$unitsIndex = defined(($disp = $qs{"Display Throughput"}))? $disp : $qs{"Display Throughput\$"};
	# unitsIndex == 0 => units = Kbits/sec | unitsIndex == 1 => units = MBits/sec
	if($unitsIndex == 1) {
		$bw *= 1024;
	}
	$data = "";
	$status = &adminprotolib::SetAttribute($data, $messageHash, $authheader, $qtssip, $qtssport, $settingsPath."maximum_bandwidth", $bw);
}

if($status != 401) {
	&cgilib::PrintRedirectHeader($servername, $hostname, $hostport, $redirectpage);
}
elsif($status == 401) {
	&cgilib::PrintChallengeResponse($servername, $data, $messageHash);
}
