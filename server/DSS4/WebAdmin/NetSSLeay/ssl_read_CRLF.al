# NOTE: Derived from blib/lib/Net/SSLeay.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Net::SSLeay;

#line 1120 "blib/lib/Net/SSLeay.pm (autosplit into blib/lib/auto/Net/SSLeay/ssl_read_CRLF.al)"
# ssl_read_CRLF($ssl [, $max_length])
sub ssl_read_CRLF { ssl_read_until($_[0], chr(13).chr(10), $_[1]) }

# ssl_write_CRLF($ssl, $message) writes $message and appends CRLF
# end of Net::SSLeay::ssl_read_CRLF
1;
