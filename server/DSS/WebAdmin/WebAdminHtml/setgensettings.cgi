#!/usr/bin/perl

require ("./adminprotocol-lib.pl");
require ("./cgi-lib.pl");

# Get the server name, QTSS IP address, and Port from ENV hash
$servername = $ENV{"SERVER_SOFTWARE"};
$qtssip = $ENV{"QTSSADMINSERVER_QTSSIP"};
$qtssport = $ENV{"QTSSADMINSERVER_QTSSPORT"};
$remoteip = $ENV{"REMOTE_HOST"};

$hostname = $ENV{"SERVER_NAME"};
$hostport = $ENV{"SERVER_PORT"};
$redirectpage = "parse.cgi?settings/settings_general.htm";

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
	$data = "";
	$settingsPath = "/admin/server/qtssSvrPreferences/";
	$attrValue = $qs{$setting};
	$passwordPath = "/admin/server/qtssSvrModuleObjects/QTSSAdminModule/qtssModPrefs/AdministratorPassword";
	
	if($setting eq "Movies Directory") {
		$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $settingsPath."movie_folder", $attrValue);		
	}
	elsif($setting eq "Server Name") {
		$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $settingsPath."default_authorization_realm", $attrValue);
	}
	elsif($setting eq "Maximum Number of Connections") {
		$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $settingsPath."maximum_connections", $attrValue);
	}
	elsif($setting eq "Streaming on Port 80") {
		if($attrValue eq "on") {
			$status = &adminprotolib::AddValueToAttribute($data, $authheader, $qtssip, $qtssport, $settingsPath."rtsp_port", 80);				
		}
		elsif($attrValue eq "off") {
			$status = &adminprotolib::DeleteValueFromAttribute($data, $authheader, $qtssip, $qtssport, $settingsPath."rtsp_port", 80);
		}
	}
	elsif($setting eq "Administrator Password") {
		$status = &adminprotolib::SetPassword($data, $authheader, $qtssip, $qtssport, $passwordPath, $attrValue);
	}
	if($status == 401) {
		last;
	}
}

# If either the value or the units or both changed, set the value
if(($status != 401) && ($qs{"Maximum Throughput"} || $qs{"Display Throughput"})) {
	$bw = defined(($val = $qs{"Maximum Throughput"}))? $val : $qs{"Maximum Throughput\$"};
	$units = defined(($disp = $qs{"Display Throughput"}))? $disp : $qs{"Display Throughput\$"};
	if($units eq "Mbps") {
		$bw *= 1024;
	}
	$data = "";
	$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $settingsPath."maximum_bandwidth", $bw);
}

if($status != 401) {
	&cgilib::PrintRedirectHeader($servername, $hostname, $hostport, $redirectpage);
}
elsif($status == 401) {
	&cgilib::PrintChallengeResponse($servername, $data);
}
