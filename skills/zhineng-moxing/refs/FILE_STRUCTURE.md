# 智能模型 · 逻辑结构

RK侧：camera → preprocess → infer → post → uart_tx  
MCU侧：uart_rx → pid → pwm；enc/imu 闭环  
配置：模型 rknn、阈值/类名、串口波特率、PID。
