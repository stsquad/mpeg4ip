# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1059 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/ssl_write_all.al)"
sub ssl_write_all {
    my $ssl = $_[0];    
    my ($data_ref, $errs);
    if (ref $_[1]) {
	$data_ref = $_[1];
    } else {
	$data_ref = \$_[1];
    }
    my ($wrote, $written, $to_write) = (0,0, length($$data_ref));
    $vm = (split ' ', `cat /proc/$$/stat`)[22] if $trace>2;  # Linux Only?
    warn "  write_all VM at entry=$vm\n" if $trace>2;
    do {
	#sleep 1; # *** DEBUG
	warn "partial `$$data_ref'\n" if $trace>3;
	$wrote = write_partial($ssl, $written, $to_write, $$data_ref);
	$written += $wrote;
	$to_write -= $wrote;
	$vm = (split ' ', `cat /proc/$$/stat`)[22] if $trace>2;  # Linux Only?
	warn "  written so far $wrote:$written bytes (VM=$vm)\n" if $trace>2;
	return (wantarray ? (undef, $errs) : undef)
	    if $errs = print_errs('SSL_write');
    } while ($to_write);
    return wantarray ? ($written, $errs) : $written;
}

# end of Net::SSLeay::ssl_write_all
1;
