#ifndef __HEADFILE_H_
#define __HEADFILE_H_

#include <opencv2/opencv.hpp>
#include <json/json.h>
#include <sys/socket.h>
#include <arpa/inet.h>  
#include <unistd.h>     
#include <iostream>
#include <chrono>
#include <pthread.h>
#include <time.h>
#include <fstream>
#include <cmath>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <cstdint>
#include <thread>
#include <mutex>
#include <linux/input.h>

#include "device_keyCtl.h"
#include "device_fans.h"
#include "device_icm20602.h"
#include "device_motor.h"
#include "device_servo.h"
#include "device_tft180.h"

#include "control_FuzzyCtrl.h"
#include "control_pid.h"
#include "control_thread.h"
#include "control_ackerman.h"
#include "control_lcdshow.h"

#include "tracking_base.h"
#include "tracking_crossRoad.h"
#include "tracking_ring.h"

#include "common_menu.h"
#include "common_Mymenu.h"
#include "preprocess.h"

using namespace std;
using namespace cv;

#define COLSIMAGE 160
#define ROWSIMAGE 100

extern double duration;

#endif
