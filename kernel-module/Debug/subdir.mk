################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../syscalls_mon.c \
../syscalls_mon_reader.c \
../test_mon.c \
../test_reader.c 

OBJS += \
./syscalls_mon.o \
./syscalls_mon_reader.o \
./test_mon.o \
./test_reader.o 

C_DEPS += \
./syscalls_mon.d \
./syscalls_mon_reader.d \
./test_mon.d \
./test_reader.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


