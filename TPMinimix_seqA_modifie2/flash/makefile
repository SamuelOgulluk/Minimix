################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := TPMinimix_seqA_modifie2
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: TPMinimix_seqA_modifie2.axf

# Tool invocations
TPMinimix_seqA_modifie2.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Users\samue\Documents\MCUXpressoIDE_25.6.136\workspace\peripherals_lib\flash" -L"C:\Users\samue\Documents\MCUXpressoIDE_25.6.136\workspace\utilities_lib\flash" -Xlinker -Map="TPMinimix_seqA_modifie2.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m0 -mthumb -T TPMinimix_seqA_modifie2_flash.ld -o "TPMinimix_seqA_modifie2.axf" $(OBJS) $(USER_OBJS) $(LIBS) -lperipherals_lib -lutilities_lib
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) TPMinimix_seqA_modifie2.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "TPMinimix_seqA_modifie2.axf"; # arm-none-eabi-objcopy -v -O binary "TPMinimix_seqA_modifie2.axf" "TPMinimix_seqA_modifie2.bin" ; # checksum -p LPC804 -d "TPMinimix_seqA_modifie2.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
