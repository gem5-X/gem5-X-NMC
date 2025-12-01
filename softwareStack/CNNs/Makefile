CC          = aarch64-linux-gnu-g++
CFLAGS      = -O3 -w -std=c++17 -fcompare-debug-second 
THREADS     = -pthread
GEM5_HOME   = ../../gem5-x-nmc
CCFLAGS     = -I$(GEM5_HOME)/include

LDFLAGS     = -L$(GEM5_HOME)/util/m5 -lm5 -lpthread

# Dirs to include
INC         = -I ./ -I ../eigen -I ./models -I ../tinytensorlib/ -I ../NMClib  

SUFFIX ?= exp

TARGETS_LIST = AlexNet VGG16CIFAR100 SSDResNet34 LeNet5MNIST

###################################
# Which benchmarks are we making? #
###################################

# Compile
$(TARGETS_LIST):
	@if [ "$@" = "SSDResNet34" ]; then \
		echo "Building special variants for SSDResNet34..."; \
		$(CC) $(CFLAGS) $(INC) $(THREADS) $(CCFLAGS) -DCNM -DBASELINE $(STATS) -Wall -DSSDRESNET34 models/$@.cpp -o $@_baseline_$(SUFFIX)$(STATS_SUFFIX) $(LDFLAGS); \
		$(CC) $(CFLAGS) $(INC) $(THREADS) $(CCFLAGS) -DCNM $(STATS) -Wall -DSSDRESNET34 models/$@.cpp -o $@_cnm_$(SUFFIX)$(STATS_SUFFIX) $(LDFLAGS); \
		$(CC) $(CFLAGS) $(INC) $(THREADS) $(CCFLAGS) -DCNM -DBASELINE -DSTATS -Wall -DSSDRESNET34 models/$@.cpp -o $@_baseline_$(SUFFIX)stats $(LDFLAGS); \
		$(CC) $(CFLAGS) $(INC) $(THREADS) $(CCFLAGS) -DCNM -DSTATS -Wall -DSSDRESNET34 models/$@.cpp -o $@_cnm_$(SUFFIX)stats $(LDFLAGS); \
	else \
		echo "Building standard target $@..."; \
		$(CC) $(CFLAGS) $(INC) $(THREADS) $(CCFLAGS) -DCNM $(STATS) -Wall -D$(shell echo '$@' | tr '[:lower:]' '[:upper:]') models/$@.cpp -o $@_breakdown_$(SUFFIX)$(STATS_SUFFIX) $(LDFLAGS); \
	fi

all: $(TARGETS_LIST)
	@echo "Compiled ALL models."

all_stats:
	$(MAKE) STATS="-DSTATS" STATS_SUFFIX="stats" $(TARGETS_LIST)

clean:
	rm -f ./*_$(SUFFIX)*
