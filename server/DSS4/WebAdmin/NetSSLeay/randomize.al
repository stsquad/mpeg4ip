# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1152 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/randomize.al)"
### Arrange some randomness for eay PRNG

sub randomize {
    my ($rn_seed_file, $seed) = @_;

    RAND_seed(rand() + $$);  # Stir it with time and pid
    
    unless (-r $rn_seed_file || -r $Net::SSLeay::random_device || $seed) {
	warn "Random number generator not seeded!!!\n" if $trace;
    }
    
    RAND_load_file($rn_seed_file, -s _) if -r $rn_seed_file;
    RAND_seed($seed) if $seed;
    RAND_load_file($Net::SSLeay::random_device, $Net::SSLeay::how_random/8)
	if -r $Net::SSLeay::random_device;
}

# end of Net::SSLeay::randomize
1;
