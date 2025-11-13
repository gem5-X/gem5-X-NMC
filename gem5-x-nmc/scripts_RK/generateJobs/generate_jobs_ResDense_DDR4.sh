#!/bin/bash
MAX_NUM_JOBS=10


GEM5X_PATH=/home/kodra/shares/local/scrap/gem5-x-cnm

LOG_FOLDER=/home/kodra/shares/local/scrap/gem5-x-cnm/logs_ResDenseDDR4

# CONF_NAMES=("librispeech_4_0_0_0_0_SW")
# EXEC_FILES=("./librispeech_4_SW.exe")

# Change directory if different DRAM
# Default:  SSDResNet
# DDR4:     SSDResNet_DDR4
# GDDR5:     SSDResNet_GDDR5
# LPDDR4:     SSDResNet_LPDDR4

APPLICATIONS_DIR=/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_DDR4

wait-n ()
{ StartJobs="$(jobs -p)"
  CurJobs="$(jobs -p)"
  while diff -q  <(echo -e "$StartJobs") <(echo -e "$CurJobs") >/dev/null
  do
    sleep 1
    CurJobs="$(jobs -p)"
  done
}
                    
for i in "$APPLICATIONS_DIR"/* ; do
    (
        application=$(basename "$i")
        # SSDResNet if default
        exec="./SSDRes_Dense_DDR4/$application" 
        echo "----------------------------------------------------"
        echo "RUNNING SIMULATION $application "
        echo "----------------------------------------------------"

        LOG_FILE="$LOG_FOLDER/$application-$(date +%F_%T).txt"

        # Use the Python -u option to see the LOG_FILE output in real time in the terminal
        /usr/bin/python3 -u $GEM5X_PATH/scripts_RK/runFSpython/run_fs_sim_command_DDR4.py $application $exec  2>&1 | tee $LOG_FILE
    ) &

    sleep 10

    # Allow to execute up to $MAX_NUM_JOBS jobs in parallel
    if [[ $(jobs -r -p | wc -l) -ge $MAX_NUM_JOBS ]]; then
        # Now there are $MAX_NUM_JOBS jobs already running, so wait here for any job
        # to be finished so there is a place to start next one.
        wait-n
    fi
done

# No more jobs to be started but wait for all pending jobs
wait

echo "All jobs finished!"

