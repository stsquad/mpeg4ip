#!/usr/bin/perl

require ("./adminprotocol-lib.pl");
require ("./cgi-lib.pl");

# Get the server name, QTSS IP address, and Port from ENV hash
$servername = $ENV{"SERVER_SOFTWARE"};
$qtssip = $ENV{"QTSSADMINSERVER_QTSSIP"};
$qtssport = $ENV{"QTSSADMINSERVER_QTSSPORT"};
$remoteip = $ENV{"REMOTE_HOST"};

$auth = $ENV{"HTTP_AUTHORIZATION"};
$authheader = "";
if(defined($auth)) {
	$authheader = "Authorization: $auth\r\n";  
}

#Get the filename from argv
$filename = $ARGV[0];
$data = "";
$status = &adminprotolib::ParseFile($data, $authheader, $qtssip, $qtssport, $filename, "IFREMOTE",  "ipaddress", $remoteip);

if($status == 401) {
	&cgilib::PrintChallengeResponse($servername, $data);
}
elsif(($filename ne "nav.htm") && ($status eq "500")) {
	&cgilib::PrintOKTextHeader($servername);
	&cgilib::PrintServerNotRunningHtml();
}
else {
	&cgilib::PrintOKTextHeader($servername);
	print $data;
}