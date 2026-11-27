################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/vision_calibration_protocol.c" \
"../code/vision_calibration_store.c" \
"../code/vision_ips_fast.c" \
"../code/vision_shared.c" \
"../code/vision_tag_tracker.c" 

COMPILED_SRCS += \
"code/vision_calibration_protocol.src" \
"code/vision_calibration_store.src" \
"code/vision_ips_fast.src" \
"code/vision_shared.src" \
"code/vision_tag_tracker.src" 

C_DEPS += \
"./code/vision_calibration_protocol.d" \
"./code/vision_calibration_store.d" \
"./code/vision_ips_fast.d" \
"./code/vision_shared.d" \
"./code/vision_tag_tracker.d" 

OBJS += \
"code/vision_calibration_protocol.o" \
"code/vision_calibration_store.o" \
"code/vision_ips_fast.o" \
"code/vision_shared.o" \
"code/vision_tag_tracker.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/vision_calibration_protocol.src":"../code/vision_calibration_protocol.c" "code/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/vision_calibration_protocol.o":"code/vision_calibration_protocol.src" "code/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/vision_calibration_store.src":"../code/vision_calibration_store.c" "code/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/vision_calibration_store.o":"code/vision_calibration_store.src" "code/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/vision_ips_fast.src":"../code/vision_ips_fast.c" "code/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/vision_ips_fast.o":"code/vision_ips_fast.src" "code/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/vision_shared.src":"../code/vision_shared.c" "code/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/vision_shared.o":"code/vision_shared.src" "code/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/vision_tag_tracker.src":"../code/vision_tag_tracker.c" "code/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 "-fD:/CardingCode/km13/km1/km13_tc387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/vision_tag_tracker.o":"code/vision_tag_tracker.src" "code/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code

clean-code:
	-$(RM) ./code/vision_calibration_protocol.d ./code/vision_calibration_protocol.o ./code/vision_calibration_protocol.src ./code/vision_calibration_store.d ./code/vision_calibration_store.o ./code/vision_calibration_store.src ./code/vision_ips_fast.d ./code/vision_ips_fast.o ./code/vision_ips_fast.src ./code/vision_shared.d ./code/vision_shared.o ./code/vision_shared.src ./code/vision_tag_tracker.d ./code/vision_tag_tracker.o ./code/vision_tag_tracker.src

.PHONY: clean-code

