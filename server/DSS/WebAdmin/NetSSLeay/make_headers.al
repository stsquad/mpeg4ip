# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1345 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/make_headers.al)"
sub make_headers {
    my (@headers) = @_;
    my $headers;
    while (@headers) {
	$headers .= shift(@headers) . ': ' . shift(@headers) . "\r\n";
    }
    return $headers;
}

# end of Net::SSLeay::make_headers
1;
