#
#	CUDA with minimum gcc version 4.7 is required (tested on CUDA 7.5).
#	



################################################################################
#
#																		Targets
#
################################################################################



.SECONDEXPANSION:
all: release



.SECONDEXPANSION:
full: release debug all_tests



#
#	Exe file with all optimisations (also unsafe ones).
#
.SECONDEXPANSION:
release: $$(RELEASE_DIR)/microflow  stl2vtk tags
release: NVCC += $(OPTIMISATION_FLAGS) -Xcompiler -ggdb -lineinfo

#
# Exe file with debug symbols and with/without optimisations.
#
.SECONDEXPANSION:
debug: $$(DEBUG_DIR)/microflow  tags
debug: NVCC += $(DEBUG_FLAGS)

#
#	Two sets of tests required - with and without unsafe optimisations.
#
test: all_tests  tags


#
#	Tags for vim.
#
tags:
	ctags -R --extra=+q --langmap=c++:+.tcc -f $(SRC_DIR)/tags --tag-relative $(SRC_DIR)


#
#	Requires cvmlcpp library
#
.SECONDEXPANSION:
stl2vtk: $$(RELEASE_DIR)/stl2vtk tags
stl2vtk: NVCC += -Xcompiler -ggdb 


################################################################################
#
#																		Settings
#
################################################################################

#
#	Compiler - all files are compiled using nvcc.
#
NVCC = nvcc -w -D_MWAITXINTRIN_H_INCLUDED -D_FORCE_INLINES -std=c++11 $(WARNINGS_C) $(INCLUDES)

NVCC_ARCH_20 = --generate-code arch=compute_20,code=sm_21
NVCC_ARCH_30 = --generate-code arch=compute_30,code=sm_35
NVCC_ARCH = $(NVCC_ARCH_20) $(NVCC_ARCH_30)

WARNINGS_C   = --compiler-options -Wall,-Wno-deprecated,-Wno-deprecated-declarations
WARNINGS_CPP = --compiler-options -Wextra,-Wno-missing-field-initializers,-Wnon-virtual-dtor,-Wdisabled-optimization,-Wpointer-arith,-Wcast-qual

OPTIMISATION_FLAGS =  -O3 --fmad=true --use_fast_math  -D NDEBUG  -D ENABLE_UNSAFE_OPTIMIZATIONS
DEBUG_FLAGS =  -g --generate-line-info -Xcompiler -ggdb


#
#	Directories.
#
BUILD_DIR   = build
CONF_DIR    = $(BUILD_DIR)/build_configuration
DEBUG_DIR   = $(BUILD_DIR)/debug
RELEASE_DIR = $(BUILD_DIR)/release

SRC_DIR         = src
APP_SRC_DIR     = $(SRC_DIR)/apps

OBJ_DIR         = obj
DEP_DIR         = obj

RELEASE_OBJ_DIR = $(RELEASE_DIR)/$(OBJ_DIR)
DEBUG_OBJ_DIR   = $(DEBUG_DIR)/$(OBJ_DIR)

RELEASE_DEP_DIR = $(RELEASE_DIR)/$(DEP_DIR)
DEBUG_DEP_DIR   = $(DEBUG_DIR)/$(DEP_DIR)

$(shell mkdir -p $(RELEASE_OBJ_DIR) >/dev/null)
$(shell mkdir -p $(RELEASE_DEP_DIR) >/dev/null)
$(shell mkdir -p $(DEBUG_OBJ_DIR) >/dev/null)
$(shell mkdir -p $(DEBUG_DEP_DIR) >/dev/null)

MAKE_ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

#
#	Ruby locations.
#
RUBY_ENV_EXTRACTOR = $(CONF_DIR)/ruby_env_extractor.rb

RUBY_INCLUDES = $(shell $(RUBY_ENV_EXTRACTOR) "INCLUDES")
RUBY_LIB      = $(shell $(RUBY_ENV_EXTRACTOR) "LIBRARY_PATH") \
					      $(shell $(RUBY_ENV_EXTRACTOR) "LIBRARIES")

#
#	mruby locations.
#
MRUBY_DIR = third_party/mruby-1.3.0

MRUBY_LIB = -L$(MRUBY_LIBRARY_PATH) -lmruby

$(MRUBY_DIR)/mruby_settings: $(MRUBY_DIR)/Makefile
	cd $(MRUBY_DIR) && make 
	cd $(MAKE_ROOT_DIR)

include $(MRUBY_DIR)/mruby_settings

#
#	VTK locations.
#
VTK_SETTINGS_DIR = third_party/vtk

$(VTK_SETTINGS_DIR)/tmp/vtk_settings: $(VTK_SETTINGS_DIR)/CMakeLists.txt
	cd $(VTK_SETTINGS_DIR)/tmp && cmake .. 
	cd $(MAKE_ROOT_DIR)

include $(VTK_SETTINGS_DIR)/tmp/vtk_settings

VTK_LIB = $(VTK_LIBRARY_PATH) -lvtkCommon -lvtkRendering -lvtkGraphics -lvtkFiltering -lvtkIO

#
#	Gzstream locations.
#
GZSTREAM_DIR = third_party/gzstream

#
#	Png++ locations.
#
PNG++_DIR = third_party/png++-0.2.5

#
#	Google tests location.
#
GTEST_DIR = third_party/gtest-1.7.0

#
#	CVMLCPP location
#
CVMLCPP_DIR = third_party/cvmlcpp-20120422

CVMLCPP_LIB = $(CVMLCPP_DIR)/libcvmlcpp.a

#
# Includes.
#
INCLUDES = -I$(abspath $(SRC_DIR))               \
					 -I$(abspath $(GZSTREAM_DIR))          \
					 -I$(abspath $(PNG++_DIR))             \
					 -I$(abspath $(GTEST_DIR))             \
					 -I$(abspath $(GTEST_DIR)/include)     \
					 -I$(abspath $(CVMLCPP_DIR))					 \
					 $(RUBY_INCLUDES)                      \
					 $(VTK_INCLUDES)						\
					 -I$(abspath $(MRUBY_DIR)) 				\
					 -I$(abspath $(MRUBY_DIR)/include)		


#
#	File names extraction.
#					 
CUDA_SRCS1 = $(wildcard $(SRC_DIR)/*.cu)
CUDA_SRCS  = $(notdir $(CUDA_SRCS1))
CUDA_OBJS = $(CUDA_SRCS:%.cu=%.o)

CPP_SRCS1 = $(wildcard $(SRC_DIR)/*.cpp )
CPP_SRCS  = $(notdir $(CPP_SRCS1))
CPP_OBJS  = $(CPP_SRCS:%.cpp=%.o)

ALL_OBJS  = $(CUDA_OBJS) $(CPP_OBJS)



################################################################################
#
#																		Compilation
#
################################################################################

COMPILE_CUDA = $(NVCC) $(NVCC_ARCH) -c $(abspath $<) -o $@ 
COMPILE_CPP  = $(NVCC) $(WARNINGS_CPP) $(GENERATE_DEPENDENCY_OPTIONS) -c $(abspath $<) -o $@

LOG_FILE = $@.log
LOG_BEGIN = echo "BEGIN $@ `date +%s%N`" >  $(LOG_FILE)
LOG_END   = echo "END   $@ `date +%s%N`" >> $(LOG_FILE)

define PRINT_COMPILATION_SUMMARY
BEGIN_TIME=`grep BEGIN $(LOG_FILE) | cut -d\  -f 3` ; END_TIME=`grep END $(LOG_FILE) | cut -d\  -f 5` ; \
TIME=`printf '%.3f\n' $$(echo "scale=3 ; (" $$END_TIME " - " $$BEGIN_TIME ") / 1000000000" | bc -l)` ; \
echo  Finished $@  ", elapsed " $$TIME "s." ; \
echo "ELAPSED $@" $$TIME "s" >> $(LOG_FILE)
endef


#
#	Allows to set different compilation settings for each file and to 
# AUTOMATICALLY recompile after compilation settings change.
#
safe_print_file = $(shell if [ -f $(1) ] ; then cat "$(1)"; fi ;)
EXTRACT_CUDA_COMPILATION_SETTINGS = $(call safe_print_file,$(CONF_DIR)/$(@F).conf)
#
# Empty rules allows to ignore missing configuration files.
#
$(CONF_DIR)/%.o.conf: ;


#
# CUDA source compilation.
#
COMPILE_CUDA_FILE_COMMAND = $(COMPILE_CUDA) $(EXTRACT_CUDA_COMPILATION_SETTINGS) -x cu

define COMPILE_CUDA_FILE
	@echo  Compiling $@
	$(LOG_BEGIN)
	echo $(GENERATE_DEPENDENCY_CUDA) >> $(LOG_FILE)
	$(GENERATE_DEPENDENCY_CUDA)
	echo $(COMPILE_CUDA_FILE_COMMAND) >> $(LOG_FILE)
	$(COMPILE_CUDA_FILE_COMMAND)
	$(LOG_END)
	@$(PRINT_COMPILATION_SUMMARY)
endef

.SECONDEXPANSION:
$(DEBUG_OBJ_DIR)/%.o $(RELEASE_OBJ_DIR)/%.o: $(SRC_DIR)/%.cu $(CONF_DIR)/%.o.conf $$(@D)/$$(*F).d
	$(COMPILE_CUDA_FILE)


#
# C++ source compilation.
#
define COMPILE_CPP_FILE
@echo  Compiling $@
$(LOG_BEGIN)
echo $(COMPILE_CPP) >> $(LOG_FILE)
$(COMPILE_CPP)
$(LOG_END)
@$(PRINT_COMPILATION_SUMMARY)
endef

.SECONDEXPANSION:
$(DEBUG_OBJ_DIR)/%.o $(RELEASE_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $$(@D)/$$(*F).d
	$(COMPILE_CPP_FILE)

.SECONDEXPANSION:
$(DEBUG_OBJ_DIR)/%.o $(RELEASE_OBJ_DIR)/%.o: $(APP_SRC_DIR)/%.cpp $$(@D)/$$(*F).d
	$(COMPILE_CPP_FILE)


#
# Automated dependency.
#
# For automated dependency look at 
#     http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
#
# BEWARE - there are different methods of .dep files generation for cpp and cuda sources.
#

GENERATE_DEPENDENCY_CUDA    = $(NVCC) $< -M --output-directory $(@D) > $(@D)/$(*F).d
GENERATE_DEPENDENCY_OPTIONS = --compiler-options -MMD,-MP

-include $(ALL_OBJS:%.o=$(DEBUG_DEP_DIR)/%.d)
-include $(ALL_OBJS:%.o=$(RELEASE_DEP_DIR)/%.d)

$(RELEASE_DEP_DIR)/%.d: ;
.PRECIOUS: $(RELEASE_DEP_DIR)/%.d

$(DEBUG_DEP_DIR)/%.d: ;
.PRECIOUS: $(DEBUG_DEP_DIR)/%.d


#
#	Linking.
#
LDFLAGS  = $(RELEASE_LDFLAGS) $(RUBY_LIB) $(VTK_LIB) $(MRUBY_LIB) -lpng -lz #-lgcov

LINK_COMMAND = $(NVCC) -Xcompiler -fopenmp  $(LDFLAGS)  -o $@ $(abspath $^)

OBJ_DIR_PREFIX = $$(@D)/$(OBJ_DIR)/

define LINK
@echo  Linking $@
$(LOG_BEGIN)
echo $(LINK_COMMAND) >> $(LOG_FILE)
$(LINK_COMMAND)
$(LOG_END)
@$(PRINT_COMPILATION_SUMMARY)
echo "\nCompilation time:\n"
grep ELAPSED $(@D)/$(OBJ_DIR)/*.log | cut -d\: -f 2 | cut -f 2,3,4 -d\  |   \
sort -r -n -k 2 | tee -a $@.profile
endef

# Extracts the fastest, predefined order of object files.
SORTED_PREREQUISITES = $$(addprefix $(OBJ_DIR_PREFIX),$$(call safe_print_file,$$(@D)/sorted_prerequisites))


.SECONDEXPANSION:
$(RELEASE_DIR)/microflow $(DEBUG_DIR)/microflow:    \
			$(SORTED_PREREQUISITES)                       \
			$$(addprefix $(OBJ_DIR_PREFIX),microflow.o)   \
			$$(addprefix $(OBJ_DIR_PREFIX),$$(ALL_OBJS))
	$(LINK)


.SECONDEXPANSION:
$(RELEASE_DIR)/stl2vtk:				 							\
			$(RELEASE_OBJ_DIR)/stl2vtk.o 					\
			$(RELEASE_OBJ_DIR)/gzstream.o 				\
			$(RELEASE_OBJ_DIR)/Logger.o 					\
			$(RELEASE_OBJ_DIR)/PerformanceMeter.o \
			$(RELEASE_OBJ_DIR)/microflowTools.o 	\
			$(CVMLCPP_LIB)
	$(LINK)


$(CVMLCPP_LIB):
	cd $(CVMLCPP_DIR) && $(MAKE)



################################################################################
#
#	                           			Tests
#
#
# Based on http://googletest.googlecode.com/svn/trunk/make/Makefile
#
################################################################################

TEST_DIR = $(BUILD_DIR)/tests

OPTIMISED_TEST_DIR = $(TEST_DIR)/optimized

OPTIMISED_OBJ_TEST_DIR = $(OPTIMISED_TEST_DIR)/$(OBJ_DIR)

$(shell mkdir -p $(OPTIMISED_OBJ_TEST_DIR) >/dev/null)


GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_ORIG_SRCS = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.

$(OPTIMISED_OBJ_TEST_DIR)/gtest-all.o: \
			$(GTEST_DIR)/src/gtest-all.cc $(GTEST_ORIG_SRCS)
	$(COMPILE_CPP_FILE)

$(OPTIMISED_OBJ_TEST_DIR)/gtest_main.o: \
			$(GTEST_DIR)/src/gtest_main.cc $(GTEST_ORIG_SRCS)
	$(COMPILE_CPP_FILE)


TST_SRCS1 = $(wildcard $(SRC_DIR)/*.cc )
TST_SRCS  = $(notdir $(TST_SRCS1))
TST_OBJS  = $(TST_SRCS:%.cc=%.o)

ALL_TST_OBJS = $(TST_OBJS) $(ALL_OBJS)


optimised_test: $(OPTIMISED_TEST_DIR)/test  tags
optimised_test: NVCC += $(DEBUG_FLAGS) $(OPTIMISATION_FLAGS)


.SECONDEXPANSION:
all_tests: optimised_test
	$(OPTIMISED_TEST_DIR)/test


.SECONDEXPANSION:
$(OPTIMISED_TEST_DIR)/test:                    \
			$(SORTED_PREREQUISITES)                                        \
			$(OBJ_DIR_PREFIX)gtest-all.o $(OBJ_DIR_PREFIX)gtest_main.o     \
			$$(addprefix $(OBJ_DIR_PREFIX),$$(ALL_TST_OBJS))
	$(LINK)



OBJS_IN_TEST_DIRECTORIES = $(OPTIMISED_OBJ_TEST_DIR)/%.o


.SECONDEXPANSION:
$(OBJS_IN_TEST_DIRECTORIES): $(SRC_DIR)/%.cc $$(@D)/$$(*F).d
	$(COMPILE_CUDA_FILE)

.SECONDEXPANSION:
$(OBJS_IN_TEST_DIRECTORIES): $(SRC_DIR)/%.cu $(CONF_DIR)/%.o.conf $$(@D)/$$(*F).d
	$(COMPILE_CUDA_FILE)

.SECONDEXPANSION:
$(OBJS_IN_TEST_DIRECTORIES): $(SRC_DIR)/%.cpp $$(@D)/$$(*F).d
	$(COMPILE_CPP_FILE)



#
#	Automated dependency.
#
$(OPTIMISED_OBJ_TEST_DIR)/%.d: ;
.PRECIOUS: $(OPTIMISED_OBJ_TEST_DIR)/%.d

-include $(ALL_TST_OBJS:%.o=$(OPTIMISED_OBJ_TEST_DIR)/%.d)

################################################################################
#
#	                           			Cleaning
#
################################################################################

.PHONY: clean
clean:
	cd $(CVMLCPP_DIR) && $(MAKE) clean
	rm -rf $(RELEASE_OBJ_DIR) $(RELEASE_DIR)/microflow*           \
				 $(DEBUG_OBJ_DIR) $(DEBUG_DIR)/microflow*               \
				 $(RELEASE_DIR)/stl2vtk*                                \
				 $(VTK_SETTINGS_DIR)/tmp/*                              \
				 $(OPTIMISED_OBJ_TEST_DIR) $(OPTIMISED_TEST_DIR)/test*
	cd $(MRUBY_DIR) && make clean
	cd $(MAKE_ROOT_DIR)