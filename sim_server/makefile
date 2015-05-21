
FW_TARG = server.exe

ACE_ROOT = /home/bning/my_sim/libs/
ACE_INCLUDES = -I$(ACE_ROOT)/include

CXX =  g++
CC =  gcc

FW_CXXFLAGS = $(FW_CORE_CXXFLAGS) -g

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@
	@-/bin/rm -f *core*

PHONY : all

all : $(FW_TARG)

clean : 
	-@rm *.o *.exe core.* *.log 2>/dev/null
	@echo "clean completed"


SRCS = tinystr.cpp \
	tinyxml.cpp \
	tinyxmlerror.cpp \
	tinyxmlparser.cpp \
	agent_handler.cpp \
	agent_service.cpp \
	agent_request.cpp \
	agent_response.cpp \
	agent_action.cpp \
	agent_ui.cpp \
	sim_handler.cpp \
	sim_service.cpp \
	sim_map.cpp \
	sim_manager.cpp \
	sim_monitor.cpp \
	sim_scheduler.cpp \
	common_utility.cpp \
	main.cpp

OBJS = $(SRCS:.cpp=.o)


##############################################################
#  vpath directive so [clear|gnu]make can find our C++
#  source file prerequisites, which are typically
#  scattered all over Framework.  The paths here were
#  specifically ordered such that they contain from most
#  to least cpp files needed for SIM.
##############################################################

VPATH_DIRS = .

vpath %.cpp $(VPATH_DIRS)



##############################################################
#  Construct the arguments to the compiler/linker.
##############################################################

INCLUDES = $(ACE_INCLUDES)

LDFLAGS = \
        -L$(ACE_ROOT)/lib \
        -lACE \
	-lACE_Monitor_Control

CXXFLAGS = \
	$(FW_CXXFLAGS) \
	$(INCLUDES) 



##############################################################
#  Rule for building (linking) output objects
##############################################################

$(FW_TARG) : $(OBJS)
ifeq (1,${SB})
	@-/bin/echo "$(FW_TARG) source browser info generated."
else
	@if test -f $(FW_TARG) || test -h $(FW_TARG) ; then \
		/bin/rm -f $(FW_TARG) ; \
	fi
	
	$(PURIFY_OPT) $(CXX) $(OBJS) $(LDFLAGS) -o $@
	@if test -f $(FW_TARG) ; then \
		/bin/echo  ; \
		/bin/echo  $(FW_TARG) has been built successfully!; \
		/bin/echo  ; \
	fi

endif


