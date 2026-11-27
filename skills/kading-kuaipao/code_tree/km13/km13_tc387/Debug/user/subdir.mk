################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../user/car_follow.c" \
"../user/car_imu.c" \
"../user/car_nav.c" \
"../user/car_path.c" \
"../user/car_replay.c" \
"../user/car_vision.c" \
"../user/car_voice.c" \
"../user/cpu0_main.c" \
"../user/cpu1_main.c" \
"../user/cpu2_main.c" \
"../user/cpu3_main.c" \
"../user/isr.c" 

COMPILED_SRCS += \
"user/car_follow.src" \
"user/car_imu.src" \
"user/car_nav.src" \
"user/car_path.src" \
"user/car_replay.src" \
"user/car_vision.src" \
"user/car_voice.src" \
"user/cpu0_main.src" \
"user/cpu1_main.src" \
"user/cpu2_main.src" \
"user/cpu3_main.src" \
"user/isr.src" 

C_DEPS += \
"./user/car_follow.d" \
"./user/car_imu.d" \
"./user/car_nav.d" \
"./user/car_path.d" \
"./user/car_replay.d" \
"./user/car_vision.d" \
"./user/car_voice.d" \
"./user/cpu0_main.d" \
"./user/cpu1_main.d" \
"./user/cpu2_main.d" \
"./user/cpu3_main.d" \
"./user/isr.d" 

OBJS += \
"user/car_follow.o" \
"user/car_imu.o" \
"user/car_nav.o" \
"user/car_path.o" \
"user/car_replay.o" \
"user/car_vision.o" \
"user/car_voice.o" \
"user/cpu0_main.o" \
"user/cpu1_main.o" \
"user/cpu2_main.o" \
"user/cpu3_main.o" \
"user/isr.o" 


# Each subdirectory must supply rules for building sources it contributes
"user/car_follow.src":"../user/car_follow.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/car_follow.o":"user/car_follow.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/car_imu.src":"../user/car_imu.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/car_imu.o":"user/car_imu.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/car_nav.src":"../user/car_nav.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/car_nav.o":"user/car_nav.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/car_path.src":"../user/car_path.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/car_path.o":"user/car_path.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/car_replay.src":"../user/car_replay.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/car_replay.o":"user/car_replay.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/car_vision.src":"../user/car_vision.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/car_vision.o":"user/car_vision.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/car_voice.src":"../user/car_voice.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/car_voice.o":"user/car_voice.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/cpu0_main.src":"../user/cpu0_main.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/cpu0_main.o":"user/cpu0_main.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/cpu1_main.src":"../user/cpu1_main.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/cpu1_main.o":"user/cpu1_main.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/cpu2_main.src":"../user/cpu2_main.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/cpu2_main.o":"user/cpu2_main.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/cpu3_main.src":"../user/cpu3_main.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/cpu3_main.o":"user/cpu3_main.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"user/isr.src":"../user/isr.c" "user/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"user/isr.o":"user/isr.src" "user/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-user

clean-user:
	-$(RM) ./user/car_follow.d ./user/car_follow.o ./user/car_follow.src ./user/car_imu.d ./user/car_imu.o ./user/car_imu.src ./user/car_nav.d ./user/car_nav.o ./user/car_nav.src ./user/car_path.d ./user/car_path.o ./user/car_path.src ./user/car_replay.d ./user/car_replay.o ./user/car_replay.src ./user/car_vision.d ./user/car_vision.o ./user/car_vision.src ./user/car_voice.d ./user/car_voice.o ./user/car_voice.src ./user/cpu0_main.d ./user/cpu0_main.o ./user/cpu0_main.src ./user/cpu1_main.d ./user/cpu1_main.o ./user/cpu1_main.src ./user/cpu2_main.d ./user/cpu2_main.o ./user/cpu2_main.src ./user/cpu3_main.d ./user/cpu3_main.o ./user/cpu3_main.src ./user/isr.d ./user/isr.o ./user/isr.src

.PHONY: clean-user

