# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1171 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/sslcat.al)"
###
### Basic request - response primitive (don't use for https)
###

sub sslcat { # address, port, message --> returns reply
    my ($dest_serv, $port, $out_message) = @_;
    my ($ctx, $ssl, $got, $errs, $written);

    ($got, $errs) = open_tcp_connection($dest_serv, $port, \$errs);
    return (wantarray ? (undef, $errs) : undef) unless $got;
	    
    ### Do SSL negotiation stuff
	    
    warn "Creating SSL $ssl_version context...\n" if $trace>2;
    load_error_strings();         # Some bloat, but I'm after ease of use
    SSLeay_add_ssl_algorithms();  # and debuggability.
    randomize('/etc/passwd');
    
    if    ($ssl_version == 2) { $ctx = CTX_v2_new(); }
    elsif ($ssl_version == 3) { $ctx = CTX_v3_new(); }
    else                      { $ctx = CTX_new(); }

    goto cleanup2 if $errs = print_errs('CTX_new') or !$ctx;

    CTX_set_options($ctx, &OP_ALL);
    goto cleanup2 if $errs = print_errs('CTX_set_options');
    
    warn "Creating SSL connection (context was '$ctx')...\n" if $trace>2;
    $ssl = new($ctx);
    goto cleanup if $errs = print_errs('SSL_new') or !$ssl;
    
    warn "Setting fd (ctx $ctx, con $ssl)...\n" if $trace>2;
    set_fd($ssl, fileno(SSLCAT_S));
    goto cleanup if $errs = print_errs('set_fd');
    
    warn "Entering SSL negotiation phase...\n" if $trace>2;
    
    $got = Net::SSLeay::connect($ssl);
    warn "SSLeay connect returned $got\n" if $trace>2;
    goto cleanup if $errs = print_errs('SSL_connect');
    
    if ($trace>1) {	    
	warn "Cipher `" . get_cipher($ssl) . "'\n";
	print_errs('get_ciper');
	warn dump_peer_certificate($ssl);
    }
    
    ### Connected. Exchange some data (doing repeated tries if necessary).
        
    warn "sslcat $$: sending " . length($out_message) . " bytes...\n"
	if $trace==3;
    warn "sslcat $$: sending `$out_message' (" . length($out_message)
	. " bytes)...\n" if $trace>3;
    ($written, $errs) = ssl_write_all($ssl, $out_message);
    goto cleanup unless $written;
    
    sleep $slowly if $slowly;  # Closing too soon can abort broken servers
    shutdown SSLCAT_S, 1;  # Half close --> No more output, send EOF to server
    
    warn "waiting for reply...\n" if $trace>2;
    ($got, $errs) = ssl_read_all($ssl);
    warn "Got " . length($got) . " bytes.\n" if $trace==3;
    warn "Got `$got' (" . length($got) . " bytes)\n" if $trace>3;

cleanup:	    
    free ($ssl);
    $errs .= print_errs('SSL_free');
cleanup2:
    CTX_free ($ctx);
    $errs .= print_errs('CTX_free');
    close SSLCAT_S;    
    return wantarray ? ($got, $errs) : $got;
}

# end of Net::SSLeay::sslcat
1;
