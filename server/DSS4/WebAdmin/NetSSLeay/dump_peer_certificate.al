# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1140 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/dump_peer_certificate.al)"
### Quickly print out with whom we're talking

sub dump_peer_certificate {
    my ($ssl) = @_;
    my $cert = get_peer_certificate($ssl);
    return if print_errs('get_peer_certificate');
    return "Subject Name: "
	. X509_NAME_oneline(X509_get_subject_name($cert)) . "\n"
        . "Issuer  Name: "
	. X509_NAME_oneline(X509_get_issuer_name($cert))  . "\n";
}

# end of Net::SSLeay::dump_peer_certificate
1;
