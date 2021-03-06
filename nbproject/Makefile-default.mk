#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=keyinput.c texteditor.c ps2keyboard.c colortext32.c textlib.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/keyinput.o ${OBJECTDIR}/texteditor.o ${OBJECTDIR}/ps2keyboard.o ${OBJECTDIR}/colortext32.o ${OBJECTDIR}/textlib.o
POSSIBLE_DEPFILES=${OBJECTDIR}/keyinput.o.d ${OBJECTDIR}/texteditor.o.d ${OBJECTDIR}/ps2keyboard.o.d ${OBJECTDIR}/colortext32.o.d ${OBJECTDIR}/textlib.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/keyinput.o ${OBJECTDIR}/texteditor.o ${OBJECTDIR}/ps2keyboard.o ${OBJECTDIR}/colortext32.o ${OBJECTDIR}/textlib.o

# Source Files
SOURCEFILES=keyinput.c texteditor.c ps2keyboard.c colortext32.c textlib.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MX250F128B
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/keyinput.o: keyinput.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/keyinput.o.d 
	@${RM} ${OBJECTDIR}/keyinput.o 
	@${FIXDEPS} "${OBJECTDIR}/keyinput.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/keyinput.o.d" -o ${OBJECTDIR}/keyinput.o keyinput.c     
	
${OBJECTDIR}/texteditor.o: texteditor.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/texteditor.o.d 
	@${RM} ${OBJECTDIR}/texteditor.o 
	@${FIXDEPS} "${OBJECTDIR}/texteditor.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/texteditor.o.d" -o ${OBJECTDIR}/texteditor.o texteditor.c     
	
${OBJECTDIR}/ps2keyboard.o: ps2keyboard.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ps2keyboard.o.d 
	@${RM} ${OBJECTDIR}/ps2keyboard.o 
	@${FIXDEPS} "${OBJECTDIR}/ps2keyboard.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/ps2keyboard.o.d" -o ${OBJECTDIR}/ps2keyboard.o ps2keyboard.c     
	
${OBJECTDIR}/colortext32.o: colortext32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/colortext32.o.d 
	@${RM} ${OBJECTDIR}/colortext32.o 
	@${FIXDEPS} "${OBJECTDIR}/colortext32.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/colortext32.o.d" -o ${OBJECTDIR}/colortext32.o colortext32.c     
	
${OBJECTDIR}/textlib.o: textlib.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/textlib.o.d 
	@${RM} ${OBJECTDIR}/textlib.o 
	@${FIXDEPS} "${OBJECTDIR}/textlib.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/textlib.o.d" -o ${OBJECTDIR}/textlib.o textlib.c     
	
else
${OBJECTDIR}/keyinput.o: keyinput.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/keyinput.o.d 
	@${RM} ${OBJECTDIR}/keyinput.o 
	@${FIXDEPS} "${OBJECTDIR}/keyinput.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/keyinput.o.d" -o ${OBJECTDIR}/keyinput.o keyinput.c     
	
${OBJECTDIR}/texteditor.o: texteditor.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/texteditor.o.d 
	@${RM} ${OBJECTDIR}/texteditor.o 
	@${FIXDEPS} "${OBJECTDIR}/texteditor.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/texteditor.o.d" -o ${OBJECTDIR}/texteditor.o texteditor.c     
	
${OBJECTDIR}/ps2keyboard.o: ps2keyboard.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ps2keyboard.o.d 
	@${RM} ${OBJECTDIR}/ps2keyboard.o 
	@${FIXDEPS} "${OBJECTDIR}/ps2keyboard.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/ps2keyboard.o.d" -o ${OBJECTDIR}/ps2keyboard.o ps2keyboard.c     
	
${OBJECTDIR}/colortext32.o: colortext32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/colortext32.o.d 
	@${RM} ${OBJECTDIR}/colortext32.o 
	@${FIXDEPS} "${OBJECTDIR}/colortext32.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/colortext32.o.d" -o ${OBJECTDIR}/colortext32.o colortext32.c     
	
${OBJECTDIR}/textlib.o: textlib.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/textlib.o.d 
	@${RM} ${OBJECTDIR}/textlib.o 
	@${FIXDEPS} "${OBJECTDIR}/textlib.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mips16 -Os -MMD -MF "${OBJECTDIR}/textlib.o.d" -o ${OBJECTDIR}/textlib.o textlib.c     
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk  libsdfsio.a  
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mdebugger -D__MPLAB_DEBUGGER_PK3=1 -mprocessor=$(MP_PROCESSOR_OPTION) -mips16 -o dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}    libsdfsio.a          -mreserve=boot@0x1FC00490:0x1FC00BEF  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_PK3=1,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk  libsdfsio.a 
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION) -mips16 -o dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}    libsdfsio.a        -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"
	${MP_CC_DIR}/xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/ps2_editor.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
