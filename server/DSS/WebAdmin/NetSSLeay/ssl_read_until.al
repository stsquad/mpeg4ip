# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1090 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/ssl_read_until.al)"
### from patch by Clinton Wong <clintdw@netcom.com>

# ssl_read_until($ssl [, $delimit [, $max_length]])
#  if $delimit missing, use $/ if it exists, otherwise use \n
#  read until delimiter reached, up to $max_length chars if defined

sub ssl_read_until {
    my ($ssl,$delimit, $max_length) = @_;

    # guess the delimit string if missing
    if ( ! defined $delimit ) {           
      if ( defined $/ && length $/  ) { $delimit = $/ }
      else { $delimit = "\n" }      # Note: \n,$/ value depends on the platform
    }
    my $length_delimit = length $delimit;

    my ($reply, $got);
    do {
        $got = Net::SSLeay::read($ssl,1);
        last if print_errs('SSL_read');
        $vm = (split ' ', `cat /proc/$$/stat`)[22] if $trace>1;  # Linux Only?
        warn "  got " . length($got) . ':'
            . length($reply) . " bytes (VM=$vm).\n" if $trace == 2;
        warn "  got `$got' (" . length($got) . ':'
            . length($reply) . " bytes, VM=$vm)\n" if $trace>2;
        $reply .= $got;
    } while ($got &&
              ( $length_delimit==0 || substr($reply, length($reply)-
                $length_delimit) ne $delimit
              ) &&
              (!defined $max_length || length $reply < $max_length)
            );
    return $reply;
}

# end of Net::SSLeay::ssl_read_until
1;
