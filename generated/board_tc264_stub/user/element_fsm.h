#ifndef USER_ELEMENT_FSM_H_
#define USER_ELEMENT_FSM_H_

typedef enum {
  EL_STRAIGHT = 0,
  EL_CURVE,
  EL_ROUNDABOUT,
  EL_CROSS,
  EL_STOP
} ElementState;

typedef struct {
  ElementState state;
  float v_ref;
} ElementOut;

void element_fsm_init(void);
ElementOut element_fsm_step(float err, float sum_adc);

#endif
