# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1356 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/do_https.al)"
# ($page, $respone_or_err, %headers) = do_https(...);

sub do_https {
    my ($site, $port, $path, $method, $headers, $content, $mime_type) = @_;
    my ($response, $page, $errs, $http, $h,$v);

    if ($content) {
	$mime_type = "application/x-www-form-urlencoded" unless $mime_type;
	my $len = length($content);
	$content = "Content-Type: $mime_type\r\n"
	    . "Content-Length: $len\r\n\r\n$content";
    } else {
	$content = "\r\n\r\n";
    }
    my $req = "$method $path HTTP/1.0\r\nHost: $site:$port\r\n"
      . $headers . "Accept: */*\r\n$content";    

    ($http, $errs) = https_cat($site, $port, $req);    
    return (undef, "HTTP/1.0 900 NET OR SSL ERROR\r\n\r\n$errs") if $errs;
    
    ($headers, $page) = split /\s?\n\s?\n/, $http, 2;
    ($response, $headers) = split /\s?\n/, $headers, 2;
    return ($page, $response,
	    map( { ($h,$v)=/^(\S+)\:\s*(.*)$/; (uc($h),$v); }
		split(/\s?\n/, $headers)
		)
	    );
}

# end of Net::SSLeay::do_https
1;
