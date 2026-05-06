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
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../modem/app/app.c ../modem/board/board.c ../modem/drivers/spi.c ../modem/drivers/timer.c ../modem/drivers/uart.c ../modem/main.c ../modem/protocol/protocol.c ../modem/radio/cc1120.c ../modem/radio/radio_link.c ../modem/util/crc8.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/2069575635/app.p1 ${OBJECTDIR}/_ext/291444238/board.p1 ${OBJECTDIR}/_ext/963368279/spi.p1 ${OBJECTDIR}/_ext/963368279/timer.p1 ${OBJECTDIR}/_ext/963368279/uart.p1 ${OBJECTDIR}/_ext/2109966821/main.p1 ${OBJECTDIR}/_ext/591255540/protocol.p1 ${OBJECTDIR}/_ext/277082361/cc1120.p1 ${OBJECTDIR}/_ext/277082361/radio_link.p1 ${OBJECTDIR}/_ext/268264310/crc8.p1
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/2069575635/app.p1.d ${OBJECTDIR}/_ext/291444238/board.p1.d ${OBJECTDIR}/_ext/963368279/spi.p1.d ${OBJECTDIR}/_ext/963368279/timer.p1.d ${OBJECTDIR}/_ext/963368279/uart.p1.d ${OBJECTDIR}/_ext/2109966821/main.p1.d ${OBJECTDIR}/_ext/591255540/protocol.p1.d ${OBJECTDIR}/_ext/277082361/cc1120.p1.d ${OBJECTDIR}/_ext/277082361/radio_link.p1.d ${OBJECTDIR}/_ext/268264310/crc8.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/2069575635/app.p1 ${OBJECTDIR}/_ext/291444238/board.p1 ${OBJECTDIR}/_ext/963368279/spi.p1 ${OBJECTDIR}/_ext/963368279/timer.p1 ${OBJECTDIR}/_ext/963368279/uart.p1 ${OBJECTDIR}/_ext/2109966821/main.p1 ${OBJECTDIR}/_ext/591255540/protocol.p1 ${OBJECTDIR}/_ext/277082361/cc1120.p1 ${OBJECTDIR}/_ext/277082361/radio_link.p1 ${OBJECTDIR}/_ext/268264310/crc8.p1

# Source Files
SOURCEFILES=../modem/app/app.c ../modem/board/board.c ../modem/drivers/spi.c ../modem/drivers/timer.c ../modem/drivers/uart.c ../modem/main.c ../modem/protocol/protocol.c ../modem/radio/cc1120.c ../modem/radio/radio_link.c ../modem/util/crc8.c



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
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=18F26Q10
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/2069575635/app.p1: ../modem/app/app.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/2069575635" 
	@${RM} ${OBJECTDIR}/_ext/2069575635/app.p1.d 
	@${RM} ${OBJECTDIR}/_ext/2069575635/app.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/2069575635/app.p1 ../modem/app/app.c 
	@-${MV} ${OBJECTDIR}/_ext/2069575635/app.d ${OBJECTDIR}/_ext/2069575635/app.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/2069575635/app.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/291444238/board.p1: ../modem/board/board.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/291444238" 
	@${RM} ${OBJECTDIR}/_ext/291444238/board.p1.d 
	@${RM} ${OBJECTDIR}/_ext/291444238/board.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/291444238/board.p1 ../modem/board/board.c 
	@-${MV} ${OBJECTDIR}/_ext/291444238/board.d ${OBJECTDIR}/_ext/291444238/board.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/291444238/board.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/963368279/spi.p1: ../modem/drivers/spi.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/963368279" 
	@${RM} ${OBJECTDIR}/_ext/963368279/spi.p1.d 
	@${RM} ${OBJECTDIR}/_ext/963368279/spi.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/963368279/spi.p1 ../modem/drivers/spi.c 
	@-${MV} ${OBJECTDIR}/_ext/963368279/spi.d ${OBJECTDIR}/_ext/963368279/spi.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/963368279/spi.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/963368279/timer.p1: ../modem/drivers/timer.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/963368279" 
	@${RM} ${OBJECTDIR}/_ext/963368279/timer.p1.d 
	@${RM} ${OBJECTDIR}/_ext/963368279/timer.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/963368279/timer.p1 ../modem/drivers/timer.c 
	@-${MV} ${OBJECTDIR}/_ext/963368279/timer.d ${OBJECTDIR}/_ext/963368279/timer.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/963368279/timer.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/963368279/uart.p1: ../modem/drivers/uart.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/963368279" 
	@${RM} ${OBJECTDIR}/_ext/963368279/uart.p1.d 
	@${RM} ${OBJECTDIR}/_ext/963368279/uart.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/963368279/uart.p1 ../modem/drivers/uart.c 
	@-${MV} ${OBJECTDIR}/_ext/963368279/uart.d ${OBJECTDIR}/_ext/963368279/uart.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/963368279/uart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/2109966821/main.p1: ../modem/main.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/2109966821" 
	@${RM} ${OBJECTDIR}/_ext/2109966821/main.p1.d 
	@${RM} ${OBJECTDIR}/_ext/2109966821/main.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/2109966821/main.p1 ../modem/main.c 
	@-${MV} ${OBJECTDIR}/_ext/2109966821/main.d ${OBJECTDIR}/_ext/2109966821/main.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/2109966821/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/591255540/protocol.p1: ../modem/protocol/protocol.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/591255540" 
	@${RM} ${OBJECTDIR}/_ext/591255540/protocol.p1.d 
	@${RM} ${OBJECTDIR}/_ext/591255540/protocol.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/591255540/protocol.p1 ../modem/protocol/protocol.c 
	@-${MV} ${OBJECTDIR}/_ext/591255540/protocol.d ${OBJECTDIR}/_ext/591255540/protocol.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/591255540/protocol.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/277082361/cc1120.p1: ../modem/radio/cc1120.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/277082361" 
	@${RM} ${OBJECTDIR}/_ext/277082361/cc1120.p1.d 
	@${RM} ${OBJECTDIR}/_ext/277082361/cc1120.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/277082361/cc1120.p1 ../modem/radio/cc1120.c 
	@-${MV} ${OBJECTDIR}/_ext/277082361/cc1120.d ${OBJECTDIR}/_ext/277082361/cc1120.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/277082361/cc1120.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/277082361/radio_link.p1: ../modem/radio/radio_link.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/277082361" 
	@${RM} ${OBJECTDIR}/_ext/277082361/radio_link.p1.d 
	@${RM} ${OBJECTDIR}/_ext/277082361/radio_link.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/277082361/radio_link.p1 ../modem/radio/radio_link.c 
	@-${MV} ${OBJECTDIR}/_ext/277082361/radio_link.d ${OBJECTDIR}/_ext/277082361/radio_link.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/277082361/radio_link.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/268264310/crc8.p1: ../modem/util/crc8.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/268264310" 
	@${RM} ${OBJECTDIR}/_ext/268264310/crc8.p1.d 
	@${RM} ${OBJECTDIR}/_ext/268264310/crc8.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=pickit4   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/268264310/crc8.p1 ../modem/util/crc8.c 
	@-${MV} ${OBJECTDIR}/_ext/268264310/crc8.d ${OBJECTDIR}/_ext/268264310/crc8.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/268264310/crc8.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
else
${OBJECTDIR}/_ext/2069575635/app.p1: ../modem/app/app.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/2069575635" 
	@${RM} ${OBJECTDIR}/_ext/2069575635/app.p1.d 
	@${RM} ${OBJECTDIR}/_ext/2069575635/app.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/2069575635/app.p1 ../modem/app/app.c 
	@-${MV} ${OBJECTDIR}/_ext/2069575635/app.d ${OBJECTDIR}/_ext/2069575635/app.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/2069575635/app.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/291444238/board.p1: ../modem/board/board.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/291444238" 
	@${RM} ${OBJECTDIR}/_ext/291444238/board.p1.d 
	@${RM} ${OBJECTDIR}/_ext/291444238/board.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/291444238/board.p1 ../modem/board/board.c 
	@-${MV} ${OBJECTDIR}/_ext/291444238/board.d ${OBJECTDIR}/_ext/291444238/board.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/291444238/board.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/963368279/spi.p1: ../modem/drivers/spi.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/963368279" 
	@${RM} ${OBJECTDIR}/_ext/963368279/spi.p1.d 
	@${RM} ${OBJECTDIR}/_ext/963368279/spi.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/963368279/spi.p1 ../modem/drivers/spi.c 
	@-${MV} ${OBJECTDIR}/_ext/963368279/spi.d ${OBJECTDIR}/_ext/963368279/spi.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/963368279/spi.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/963368279/timer.p1: ../modem/drivers/timer.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/963368279" 
	@${RM} ${OBJECTDIR}/_ext/963368279/timer.p1.d 
	@${RM} ${OBJECTDIR}/_ext/963368279/timer.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/963368279/timer.p1 ../modem/drivers/timer.c 
	@-${MV} ${OBJECTDIR}/_ext/963368279/timer.d ${OBJECTDIR}/_ext/963368279/timer.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/963368279/timer.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/963368279/uart.p1: ../modem/drivers/uart.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/963368279" 
	@${RM} ${OBJECTDIR}/_ext/963368279/uart.p1.d 
	@${RM} ${OBJECTDIR}/_ext/963368279/uart.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/963368279/uart.p1 ../modem/drivers/uart.c 
	@-${MV} ${OBJECTDIR}/_ext/963368279/uart.d ${OBJECTDIR}/_ext/963368279/uart.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/963368279/uart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/2109966821/main.p1: ../modem/main.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/2109966821" 
	@${RM} ${OBJECTDIR}/_ext/2109966821/main.p1.d 
	@${RM} ${OBJECTDIR}/_ext/2109966821/main.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/2109966821/main.p1 ../modem/main.c 
	@-${MV} ${OBJECTDIR}/_ext/2109966821/main.d ${OBJECTDIR}/_ext/2109966821/main.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/2109966821/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/591255540/protocol.p1: ../modem/protocol/protocol.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/591255540" 
	@${RM} ${OBJECTDIR}/_ext/591255540/protocol.p1.d 
	@${RM} ${OBJECTDIR}/_ext/591255540/protocol.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/591255540/protocol.p1 ../modem/protocol/protocol.c 
	@-${MV} ${OBJECTDIR}/_ext/591255540/protocol.d ${OBJECTDIR}/_ext/591255540/protocol.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/591255540/protocol.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/277082361/cc1120.p1: ../modem/radio/cc1120.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/277082361" 
	@${RM} ${OBJECTDIR}/_ext/277082361/cc1120.p1.d 
	@${RM} ${OBJECTDIR}/_ext/277082361/cc1120.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/277082361/cc1120.p1 ../modem/radio/cc1120.c 
	@-${MV} ${OBJECTDIR}/_ext/277082361/cc1120.d ${OBJECTDIR}/_ext/277082361/cc1120.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/277082361/cc1120.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/277082361/radio_link.p1: ../modem/radio/radio_link.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/277082361" 
	@${RM} ${OBJECTDIR}/_ext/277082361/radio_link.p1.d 
	@${RM} ${OBJECTDIR}/_ext/277082361/radio_link.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/277082361/radio_link.p1 ../modem/radio/radio_link.c 
	@-${MV} ${OBJECTDIR}/_ext/277082361/radio_link.d ${OBJECTDIR}/_ext/277082361/radio_link.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/277082361/radio_link.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/268264310/crc8.p1: ../modem/util/crc8.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/_ext/268264310" 
	@${RM} ${OBJECTDIR}/_ext/268264310/crc8.p1.d 
	@${RM} ${OBJECTDIR}/_ext/268264310/crc8.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/268264310/crc8.p1 ../modem/util/crc8.c 
	@-${MV} ${OBJECTDIR}/_ext/268264310/crc8.d ${OBJECTDIR}/_ext/268264310/crc8.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/_ext/268264310/crc8.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
endif

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
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.map  -D__DEBUG=1  -mdebugger=pickit4  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto        $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}     
	@${RM} ${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.hex 
	
	
else
${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/Modem_remote.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}     
	
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
