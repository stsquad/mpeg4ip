#!/usr/bin/perl

require ("./adminprotocol-lib.pl");
require ("./cgi-lib.pl");

# Get the server name, QTSS IP address, and Port from ENV hash
$servername = $ENV{"SERVER_SOFTWARE"};
$qtssip = $ENV{"QTSSADMINSERVER_QTSSIP"};
$qtssport = $ENV{"QTSSADMINSERVER_QTSSPORT"};

$hostname = $ENV{"SERVER_NAME"};
$hostport = $ENV{"SERVER_PORT"};
$redirectpage = "parse.cgi?settings/settings_logging.htm";

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
		$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $errorPath."error_logfile_interval", $attrValue);		
	}
	elsif($setting eq "Error Log Roll Size") {
		$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $errorPath."error_logfile_size", ($attrValue * 1024));
	}
	elsif($setting eq "Access Log Roll Time") {
		$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $accessPath."request_logfile_interval", $attrValue);
	}
	elsif($setting eq "Access Log Roll Size") {
		$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $accessPath."request_logfile_size", ($attrValue * 1024));
	}
	elsif($setting eq "Log Errors") {
		if($attrValue eq "true") {
			$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $errorPath."error_logging", true);				
		}
		elsif($attrValue eq "false") {
			$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $errorPath."error_logging", false);
		}
	}
	elsif($setting eq "Log Access") {
		if($attrValue eq "true") {
			$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $accessPath."request_logging", true);				
		}
		elsif($attrValue eq "false") {
			$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, $accessPath."request_logging", false);
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
	&cgilib::PrintChallengeResponse($servername, $data);
}

