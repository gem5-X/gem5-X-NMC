import os
import re
import signal
import subprocess
import sys
import pty
from subprocess import Popen, PIPE, STDOUT

GEM5X_CMD = '/home/kodra/shares/local/scrap/gem5-x-cnm/scripts_RK/launch/launch_LPDDR4.sh'
TELNET_CMD = '/home/kodra/shares/local/scrap/gem5-x-cnm/scripts_RK/telnet_run_cmd.sh'
# SNG_CMD = 'singularity exec /home/kodra/srv11.sif'

# Change working directory to scripts folder 
abspath = os.path.abspath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)

if len(sys.argv) < 3:
    print(f"Wrong invocation. Usage '{sys.argv[0]} application_name application'")
    print(f"Instead, sys.argv={sys.argv}")
    exit(1)


application_name = sys.argv[1]
exec = sys.argv[2]
print('Executing gem5x')

# Need to create a PTY so that gem5 believes it's connected into a console and will allow telnet connections.
master_fd, slave_fd = pty.openpty()
gem5x_proc = Popen([GEM5X_CMD + ' ' + application_name], stdout=PIPE, stderr=STDOUT, stdin=slave_fd, shell=True, preexec_fn=os.setsid)

def main():

    regex = 'system.terminal: Listening for connections on port ([\\d]+)'

    while True:
        rc = gem5x_proc.poll()
        if rc is not None:
            print(f'Gem5-x exit unexpectedly with return code {rc}')
            os._exit(1) 

        line = gem5x_proc.stdout.readline().decode('utf-8')
        print(line)
        m = re.match(regex, line)
        if m is not None:
            telnet_port = m.group(1)
            print(telnet_port)
            break
    
    print(f'Starting telnet process at port {telnet_port}')

    subprocess.check_call(['singularity', 'exec', '/home/kodra/srv11.sif'] + [TELNET_CMD, telnet_port] + [exec])

    gem5x_proc.wait()

try:
    main()
except:
    print('Killing process...')
    os.killpg(os.getpgid(gem5x_proc.pid), signal.SIGTERM)
    raise