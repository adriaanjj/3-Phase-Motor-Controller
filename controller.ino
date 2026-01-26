#include <math.h>
#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"

#define LUT_SIZE 256  // Sine table size

#define P1H 12  // GPIO 12
#define P1L 14  // GPIO 14

#define DEAD_TIME_TICKS 10

// sine table
uint16_t sin_table[LUT_SIZE];

// phase index
volatile int phase_index = 0;

// timer handle
mcpwm_timer_handle_t timer = NULL;

// comparators
mcpwm_cmpr_handle_t comparators[1];

// Init MCPWM timer
void mcpwm_gpio_init(void) {
  // Init MCPWM timer
  mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = 1000000,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks = 500,
  };
  ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

  // Create MCPWM operators
  int high_pins[1] = { P1H };
  int low_pins[1] = { P1L };
  mcpwm_oper_handle_t operators[1];

  // would normally loop 3 times but just testing with 1 phase for now
  for (int i = 0; i < 1; i++) {
    // Operator, responsible for generating PWM waveforms
    mcpwm_operator_config_t operator_config = { .group_id = 0 };
    mcpwm_new_operator(&operator_config, &operators[i]);

    // Connect operators to the same timer
    mcpwm_operator_connect_timer(operators[i], timer);

    // Comparator, handles duty cycle
    mcpwm_comparator_config_t compare_config = {};
    compare_config.flags.update_cmp_on_tez = true;
    mcpwm_new_comparator(operators[i], &compare_config, &comparators[i]);

    // Connect PWM to physical pins
    mcpwm_gen_handle_t gen_h, gen_l;
    mcpwm_generator_config_t generator_config = { .gen_gpio_num = high_pins[i] };
    mcpwm_new_generator(operators[i], &generator_config, &gen_h);
    generator_config.gen_gpio_num = low_pins[i];
    mcpwm_new_generator(operators[i], &generator_config, &gen_l);

    // high at 0 low at match
    mcpwm_generator_set_action_on_timer_event(gen_h, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(gen_h, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparators[i], MCPWM_GEN_ACTION_LOW));

    mcpwm_generator_set_action_on_timer_event(gen_l, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(gen_l, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparators[i], MCPWM_GEN_ACTION_LOW));

    // dead time
    mcpwm_dead_time_config_t dt_config = {};
    dt_config.posedge_delay_ticks = DEAD_TIME_TICKS;

    // apply config to highside
    mcpwm_generator_set_dead_time(gen_h, gen_h, &dt_config);

    // apply config to lowside
    dt_config.negedge_delay_ticks = DEAD_TIME_TICKS;
    dt_config.flags.invert_output = true; 

    // make low side inverse of high side and apply config
    mcpwm_generator_set_dead_time(gen_h, gen_l, &dt_config);
  }

  mcpwm_timer_enable(timer);
  mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);
}

void generate_sine_table(uint32_t period) {
  for (int i = 0; i < LUT_SIZE; i++) {
    // scale sin wave (-1 to 1) to (0 to period)
    sin_table[i] = (uint32_t)(((sin(2.0 * M_PI * i / LUT_SIZE) + 1.0) / 2.0) * period);
  }
}

hw_timer_t *motor_timer = NULL;
void ARDUINO_ISR_ATTR on_timer() {
  phase_index = (phase_index + 1) % LUT_SIZE;
}

void setup() {
  mcpwm_gpio_init();
  generate_sine_table(500);

  motor_timer = timerBegin(10000);
  timerAttachInterrupt(motor_timer, &on_timer);
  timerAlarm(motor_timer, 200, true, 0);
}

void loop() {
  uint32_t valA = sin_table[phase_index];

  mcpwm_comparator_set_compare_value(comparators[0], valA);

  delay(1);
}
