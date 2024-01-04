# The client writes a message to Sys::Syslog native method.
# The syslogd writes it into a file and through a pipe.
# The syslogd passes it via TCP to an explicit loghost.
# The server receives the message on its TCP socket.
# Find the message in client, file, pipe, syslogd, server log.
# Check that syslogd and server log contain localhost address.
# Check that with TCP server all other sockets are closed.

use strict;
use warnings;
use Socket;

our %args = (
    syslogd => {
	loghost => '@tcp://localhost:$connectport',
	loggrep => {
	    qr/Logging to FORWTCP \@tcp:\/\/localhost:\d+/ => '>=4',
	    get_testgrep() => 1,
	},
	fstat => {
	    qr/internet/ => 1,
	    qr/^_syslogd .* stream tcp / => 1,
	},
    },
    server => {
	listen => { domain => AF_UNSPEC, proto => "tcp", addr => "localhost" },
	loggrep => {
	    qr/listen sock: (127.0.0.1|::1) \d+/ => 1,
	    get_testgrep() => 1,
	},
    },
);

1;
