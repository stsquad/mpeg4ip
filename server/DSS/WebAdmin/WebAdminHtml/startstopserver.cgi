#!/usr/bin/perl

require ("./adminprotocol-lib.pl");
require ("./cgi-lib.pl");

# Get the server name, QTSS IP address, and Port from ENV hash
$servername = $ENV{"SERVER_SOFTWARE"};
$qtssip = $ENV{"QTSSADMINSERVER_QTSSIP"};
$qtssport = $ENV{"QTSSADMINSERVER_QTSSPORT"};
$qtssname = $ENV{"QTSSADMINSERVER_QTSSNAME"};

$hostname = $ENV{"SERVER_NAME"};
$hostport = $ENV{"SERVER_PORT"};
$redirectpage = "parse.cgi?nav.htm";

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
if($setting eq "Stop Server Now") {
	$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, "/admin/server/qtssRTSPSvrState", 5);
}
elsif($setting eq "Start Server Now") {
	$status = &adminprotolib::SetAttribute($data, $authheader, $qtssip, $qtssport, "/admin/server/qtssRTSPSvrState", 1);
	if($status == 500) {
		&adminprotolib::StartServer($qtssname);
	}
}

if($status != 401) {
	&cgilib::PrintRedirectHeader($servername, $hostname, $hostport, $redirectpage);
}
elsif($status == 401) {
	&cgilib::PrintChallengeResponse($servername, $data);
}
