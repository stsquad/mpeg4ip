# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1388 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/post_https.al)"
sub post_https {
    my ($site, $port, $path, $headers, $post_str, $mime) = @_;
    return do_https($site, $port, $path, 'POST', $headers, $post_str, $mime);
}

1;
__END__
1;
# end of Net::SSLeay::post_https
