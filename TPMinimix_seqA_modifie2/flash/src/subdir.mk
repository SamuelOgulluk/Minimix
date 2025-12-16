################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ADC_Seqa_Seqb_ISRs.c \
../src/MCUXpresso_Retarget.c \
../src/MCUXpresso_cr_startup.c \
../src/MCUXpresso_crp.c \
../src/MCUXpresso_mtb.c \
../src/Serial.c \
../src/system.c \
../src/tp_minimix_modifie2.c 

C_DEPS += \
./src/ADC_Seqa_Seqb_ISRs.d \
./src/MCUXpresso_Retarget.d \
./src/MCUXpresso_cr_startup.d \
./src/MCUXpresso_crp.d \
./src/MCUXpresso_mtb.d \
./src/Serial.d \
./src/system.d \
./src/tp_minimix_modifie2.d 

OBJS += \
./src/ADC_Seqa_Seqb_ISRs.o \
./src/MCUXpresso_Retarget.o \
./src/MCUXpresso_cr_startup.o \
./src/MCUXpresso_crp.o \
./src/MCUXpresso_mtb.o \
./src/Serial.o \
./src/system.o \
./src/tp_minimix_modifie2.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -DCR_INTEGER_PRINTF -D__USE_CMSIS -D__CODE_RED -DCORE_M0PLUS -D__MTB_DISABLE -D__MTB_BUFFER_SIZE=256 -D__REDLIB__ -I"C:\Users\samue\Documents\MCUXpressoIDE_25.6.136\workspace\TPMinimix_seqA_modifie2\inc" -I"C:\Users\samue\Documents\MCUXpressoIDE_25.6.136\workspace\peripherals_lib\inc" -I"C:\Users\samue\Documents\MCUXpressoIDE_25.6.136\workspace\utilities_lib\inc" -I"C:\Users\samue\Documents\MCUXpressoIDE_25.6.136\workspace\common\inc" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0 -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/ADC_Seqa_Seqb_ISRs.d ./src/ADC_Seqa_Seqb_ISRs.o ./src/MCUXpresso_Retarget.d ./src/MCUXpresso_Retarget.o ./src/MCUXpresso_cr_startup.d ./src/MCUXpresso_cr_startup.o ./src/MCUXpresso_crp.d ./src/MCUXpresso_crp.o ./src/MCUXpresso_mtb.d ./src/MCUXpresso_mtb.o ./src/Serial.d ./src/Serial.o ./src/system.d ./src/system.o ./src/tp_minimix_modifie2.d ./src/tp_minimix_modifie2.o

.PHONY: clean-src

