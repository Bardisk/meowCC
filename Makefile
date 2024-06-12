GDB = gdb

# DIRS

WORK_DIR = $(realpath .)
BUILD_DIR = $(WORK_DIR)/build
OUT_DIR = $(WORK_DIR)/output
SRCS = $(wildcard source/*.c)
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:%.c=%.o)) 
$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(OUT_DIR))

# FLAGS

INC_PATH = include
INC_FLAG = $(addprefix -I, $(INC_PATH))
CFLAGS = -g -std=c99 -Wall -Wextra -Wshadow -Wconversion -MMD
CFLAGS += $(INC_FLAG)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@ 

-include $(OBJS:%.o=%.d)

all: $(BUILD_DIR)/meowCC

ERR_TESTS = $(wildcard err_tests/*.cm)
EXPR_TESTS = $(wildcard expr_tests/*.exp)
STMT_TESTS = $(wildcard expr_tests/*.stmt)
ALL_TESTS := $(ERR_TESTS) sample.cm

ifdef TESTNAME
NOTTEST := $(shell echo $(ALL_TESTS) | sed 's/\S*$(TESTNAME)\S*//g')
ALL_TESTS := $(filter-out $(NOTTEST), $(ALL_TESTS))
endif 

ALL_TESTS_OUTL = $(addprefix $(OUT_DIR)/, $(ALL_TESTS:%.cm=%.outl))
ALL_TESTS_ST = $(addprefix $(OUT_DIR)/, $(ALL_TESTS:%.cm=%.st))
EXPR_TESTS_ST = $(addprefix $(OUT_DIR)/, $(EXPR_TESTS:%.exp=%.st))


# lexer test
$(OUT_DIR)/%.outl: %.cm all 
	@mkdir -p $(dir $@)
ifndef DEBUGGING
	@-$(BUILD_DIR)/meowCC -l $< > $@ 2>&1; \
	if [ $$? -eq 0 ]; \
	then \
		echo -en "\e[32mACCEPT\e[0m\t:"; \
	else \
		echo -en "\e[31mERROR\e[0m\t:"; \
	fi;
else
	@-$(GDB) $(BUILD_DIR)/meowCC -ex "start -l $< > $@ 2>&1"
endif
	@echo " $(shell basename $@)"

# whole test
$(OUT_DIR)/%.st: %.cm all 
	@mkdir -p $(dir $@)
ifndef DEBUGGING
	@-$(BUILD_DIR)/meowCC $< > $@ 2>&1; \
	if [ $$? -eq 0 ]; \
	then \
		echo -en "\e[32mACCEPT\e[0m\t:"; \
	else \
		echo -en "\e[31mERROR\e[0m\t:"; \
	fi;
else
	@-$(GDB) $(BUILD_DIR)/meowCC -ex "start $< > $@ 2>&1"
endif
	@echo " $(shell basename $@)"

# expr test
$(OUT_DIR)/%.st: %.exp all
ifndef DEBUGGING
	@mkdir -p $(dir $@)
	@-$(BUILD_DIR)/meowCC -e $< > $@ 2>&1; \
	if [ $$? -eq 0 ]; \
	then \
		echo -en "\e[32mACCEPT\e[0m\t:"; \
	else \
		echo -en "\e[31mERROR\e[0m\t:"; \
	fi;
else
	@-$(GDB) $(BUILD_DIR)/meowCC -ex "start -e $< > $@ 2>&1"
endif
	@echo " $(shell basename $@)"

lexer_test: all $(ALL_TESTS_OUTL)

expr_test: all $(EXPR_TESTS_ST)

all_test: all $(ALL_TESTS_ST)


$(BUILD_DIR)/meowCC: $(OBJS)
	$(CC) $(OBJS) -o $@

# clean_outl:
# 	find . -name "*.outl" | xargs rm -f

clean:
	rm -rf build

.PHONY: all clean lexer_test expr_test all_test