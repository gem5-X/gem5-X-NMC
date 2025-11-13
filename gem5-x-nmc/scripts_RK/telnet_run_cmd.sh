#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Invalid call. Usage: './telnet_run_cmd.sh telnet_port command [args...]'"
    exit 1
fi

expect << EOF
log_user 1
spawn telnet localhost $1
expect "root@localhost:~#"
send "cd /mnt && $2 && m5 exit\r"
while 1 {
    expect {
        -re ".+" {}
        eof {exit}
    }
}
EOF