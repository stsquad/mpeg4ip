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
$numIndex = ($numEntries eq "AllEntries")? -1: ($numEntries - 1);
$data = "";

$status = &adminprotolib::ParseFile($data, $authheader, $qtssip, $qtssport, $filename, 
							"CREATESELECTFROMINPUTWITHFUNC", "numEntries", $numEntries,
							"CREATESELECTFROMINPUTWITHFUNC", "refreshInterval", $refreshInterval,
							"CREATESELECTFROMINPUTWITHFUNC", "sortOrder", $order, 
							"SORTRECORDSWITHINPUT", "sortOrder", $order,
							"FORMATDDARRAYWITHINPUT", "numEntries", $numIndex);

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
