#!/usr/bin/perl

require ("./adminprotocol-lib.pl");
require ("./cgi-lib.pl");

# Get the server name, QTSS IP address, and Port from ENV hash
$servername = $ENV{"SERVER_SOFTWARE"};
$qtssip = $ENV{"QTSSADMINSERVER_QTSSIP"};
$qtssport = $ENV{"QTSSADMINSERVER_QTSSPORT"};

$auth = $ENV{"HTTP_AUTHORIZATION"};
$authheader = "";
if(defined($auth)) {
	$authheader = "Authorization: $auth\r\n";  
}

$filename = "mainpage.htm";
$data = "";
$status = &adminprotolib::ParseFile($data, $authheader, $qtssip, $qtssport, $filename);

if($status == 401) {
	&cgilib::PrintChallengeResponse($servername, $data);
}
else {
	&cgilib::PrintOKTextHeader($servername);
	print $data;
}
