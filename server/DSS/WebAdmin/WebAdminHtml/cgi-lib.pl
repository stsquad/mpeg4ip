# cgi-lib.pl
# Common functions for writing http headers

package cgilib;
# init days and months
@weekday = ( "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" );
@month = ( "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" );

%status = ( '200' => 'OK',
			'401' => 'Unauthorized',
			'302' => 'Temporarily Unavailable',
		  );
			

# PrintOKTextHeader(servername)
sub PrintOKTextHeader {
	my $datestr = HttpDate(time());
	print "HTTP/1.0 200 OK\r\nDate: $datestr\r\nServer: $_[0]\r\nContent-Type: text/html\r\nConnection:close\r\n\r\n";
}

# PrintRedirectHeader(servername, serverip, serverport, redirectpage)
sub PrintRedirectHeader {
	my $datestr = HttpDate(time());
	print "HTTP/1.0 302 Moved Temporarily\r\nDate: $datestr\r\nServer: $_[0]\r\n"
			. "Location: http://$_[1]:$_[2]/$_[3]\r\nConnection:close\r\n\r\n";
}


# PrintChallengeHeader(servername, challengeheader)
sub PrintChallengeHeader {
	my $datestr = HttpDate(time());
	print "HTTP/1.0 401 Unauthorized\r\nDate: $datestr\r\nServer: $_[0]\r\n"
			. "Content-Type: text/html\r\nConnection:close\r\n$_[1]\r\n\r\n";
}

# PrintChallengeResponse(servername, challengeheader)
sub PrintChallengeResponse {
	PrintChallengeHeader($_[0], $_[1]);
	PrintUnauthorizedHtml();
}

# PrintStatusLine(num) 
sub PrintStatusLine {
	print "HTTP/1.0 $_[0] $status{$_[0]}\r\n"; 
}

# PrintDateAndServerStr(server)
sub PrintDateAndServerStr {
	my $datestr = HttpDate(time());
	print "Date: $datestr\r\nServer: $_[0]\r\n";
}

# PrintTextTypeAndCloseStr()
sub PrintTextTypeAndCloseStr {
	print "Content-Type: text/html\r\nConnection: close\r\n\r\n";
}

# PrintUnauthorizedHeader(servername, realm)
sub PrintUnauthorizedHeader {
	my $datestr = HttpDate(time());
 	print "HTTP/1.0 401 Unauthorized\r\nServer:$_[0]\r\nDate: $datestr\r\n"
 					. "WWW-authenticate: Basic realm=\"$_[1]\"\r\n"
 					. "Content-Type: text/html\r\nConnection: close\r\n\r\n";
}

# PrintServerNotRunningHtml()
sub PrintServerNotRunningHtml {
 	print "<HTML><HEAD><TITLE>Streaming Server is not Running</TITLE></HEAD>" 
 					. "<BODY><H2>Streaming Server is not running.</H2>"
 					. "<P>Click on 'Start Server Now' to start the server.</P></BODY></HTML>";
}

# PrintUnauthorizedHtml()
sub PrintUnauthorizedHtml {
 	print "<HTML><HEAD><TITLE>Unauthorized</TITLE></HEAD>" 
 					. "<BODY><H1>Unauthorized</H1><P>A password is required to administer this QuickTime Streaming Server.\n"
 					. "Please try again. </P></BODY></HTML>";
}

# PrintUnauthorizedResponse(servername, realm)
sub PrintUnauthorizedResponse {
	PrintUnauthorizedHeader($_[0], $_[1]);
 	PrintUnauthorizedHtml();
}

# HttpDate(timeinsecfrom1970)
sub HttpDate {
    local @tm = gmtime($_[0]);
    return sprintf "%s, %d %s %d %2.2d:%2.2d:%2.2d GMT",
    $weekday[$tm[6]], $tm[3], $month[$tm[4]], $tm[5]+1900,
    $tm[2], $tm[1], $tm[0];
}

1; #return true  