# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1040 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/ssl_read_all.al)"
###
### read and write helpers that block
###

sub ssl_read_all {
    my ($ssl,$how_much) = @_;
    $how_much = 2000000000 unless $how_much;
    my ($reply, $got, $errs);
    do {
	$got = Net::SSLeay::read($ssl,$how_much);
	last if $errs = print_errs('SSL_read');
	$how_much -= length($got);
	$vm = (split ' ', `cat /proc/$$/stat`)[22] if $trace>2;  # Linux Only?
	warn "  got " . length($got) . ':'
	    . length($reply) . " bytes (VM=$vm).\n" if $trace == 3;
	warn "  got `$got' (" . length($got) . ':'
	    . length($reply) . " bytes, VM=$vm)\n" if $trace>3;
	$reply .= $got;
	#$reply = $got;  # *** DEBUG
    } while ($got && $how_much > 0);
    return wantarray ? ($reply, $errs) : $reply;
}

# end of Net::SSLeay::ssl_read_all
1;
