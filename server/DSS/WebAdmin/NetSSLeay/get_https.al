# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1383 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/get_https.al)"
sub get_https {
    my ($site, $port, $path, $headers, $content, $mime) = @_;
    return do_https($site, $port, $path, 'GET', $headers, $content, $mime);
}

# end of Net::SSLeay::get_https
1;
