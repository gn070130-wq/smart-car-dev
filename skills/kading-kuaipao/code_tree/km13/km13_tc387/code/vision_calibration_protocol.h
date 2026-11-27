#ifndef VISION_CALIBRATION_PROTOCOL_H
#define VISION_CALIBRATION_PROTOCOL_H

/* Poll the debug UART RX FIFO and process complete CAL commands. */
void vision_calibration_protocol_poll(void);

#endif /* VISION_CALIBRATION_PROTOCOL_H */
