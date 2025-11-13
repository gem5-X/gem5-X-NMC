#!/bin/bash

GEM5_DIR="/home/kodra/shares/local/scrap/gem5-x-cnm/"
RESULTS_DIR="/home/kodra/shares/local/scrap/gem5-x-cnm/gem5_outputs/SSDRESNET CIxCO"

# Define output files
AUX_FILE="$GEM5_DIR/gem5_outputs/RESULTS/SSDRESNET_CIxCO_aux.txt"  # Aux file
CSV_FILE="$GEM5_DIR/gem5_outputs/RESULTS/SSDRESNET_CIxCO_hostSeconds.csv"      # Final CSV file

# Initialize aux file (empty it if it already exists)
> "$AUX_FILE"

# Loop over the dirs in SSDRESNET CIxCO
for d in "$RESULTS_DIR"/*/ ; do
    experiment=$(basename "$d")
    echo "Parsing $experiment"

    # Extract the dimensions and split into separate numbers
    dimensions=$(echo "$experiment" | grep -oE '_[0-9]+x[0-9]+$' | tr -d '_' | tr 'x' ',')

    # Locate the stats file in the experiment folder
    stats_file="$d/${experiment}_stats.txt"
    if [[ -f "$stats_file" ]]; then
        # Add the experiment name and dimensions to aux file
        echo -n "$experiment, $dimensions, " >> "$AUX_FILE"

        # Extract and append `host_seconds` values
        host_seconds=$(grep -e "host_seconds" "$stats_file" | \
                      sed 's/host_seconds//g' | \
                      sed 's/#.*//g' | \
                      tr -d ' ' | \
                      paste -sd ",")
        echo "$host_seconds" >> "$AUX_FILE"  # End line for the experiment
    else
        echo "Stats file not found for $experiment"
    fi
done

# Clean up the aux file to ensure no blank lines or trailing commas exist
sed -i '/^$/d' "$AUX_FILE"  # Remove blank lines
sed -i 's/,$//' "$AUX_FILE" # Remove any trailing commas at the end of lines

# Rename aux file to final CSV file
mv "$AUX_FILE" "$CSV_FILE"

echo "Results written to $CSV_FILE"
