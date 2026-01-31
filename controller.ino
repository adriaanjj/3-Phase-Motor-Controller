#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"

#define LUT_SIZE 360  // Sine table size

#define P1H 12 // GPIO 12
#define P1L 14 // GPIO 14
#define P2H 27 // GPIO 27
#define P2L 26 // GPIO 26
#define P3H 25 // GPIO 25
#define P3L 33 // GPIO 33

#define DEAD_TIME_TICKS 10
#define MCPWM_HZ 8000000
#define PWM_MAX 2000

// sine table
uint32_t sin_table[LUT_SIZE];

// phase index
volatile int phase_index = 0;

// timer handle
mcpwm_timer_handle_t timer = NULL;

// comparators
mcpwm_cmpr_handle_t comparators[3];

void generate_sine_table()
{
    uint32_t internal_peak = PWM_MAX / 2;
    uint32_t amplitude = internal_peak / 2;

    for (uint32_t i = 0; i < LUT_SIZE; i++) {
        sin_table[i] = (uint32_t)((sin((double)i * PI / 180) * amplitude) + amplitude);
    } 
}

static bool IRAM_ATTR on_timer(mcpwm_timer_handle_t t, const mcpwm_timer_event_data_t *edata, void *user_data)
{
  phase_index = (phase_index + 1) % LUT_SIZE;

  uint16_t SINE_A_PWM = sin_table[phase_index];
  uint16_t SINE_B_PWM = sin_table[(phase_index + 120) % LUT_SIZE];
  uint16_t SINE_C_PWM = sin_table[(phase_index + 240) % LUT_SIZE];

  mcpwm_comparator_set_compare_value(comparators[0], SINE_A_PWM);
  mcpwm_comparator_set_compare_value(comparators[1], SINE_B_PWM);
  mcpwm_comparator_set_compare_value(comparators[2], SINE_C_PWM);

  return false;
}

void mcpwm_gpio_init(void)
{
  // Init MCPWM timer
  mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = MCPWM_HZ,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks = PWM_MAX,
  };
  ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

  // Create MCPWM operators
  int high_pins[3] = { P1H, P2H, P3H };
  int low_pins[3] = { P1L, P2L, P3L };
  mcpwm_oper_handle_t operators[3];

  for (int i = 0; i < 3; i++) {
    // Operator, responsible for generating PWM waveforms
    mcpwm_operator_config_t operator_config = { .group_id = 0 };
    mcpwm_new_operator(&operator_config, &operators[i]);

    // Connect operators to the same timer
    mcpwm_operator_connect_timer(operators[i], timer);

    // Comparator, handles duty cycle
    mcpwm_comparator_config_t compare_config = {};
    compare_config.flags.update_cmp_on_tez = true;
    compare_config.flags.update_cmp_on_tep = true,
    mcpwm_new_comparator(operators[i], &compare_config, &comparators[i]);

    // Connect PWM to physical pins
    mcpwm_gen_handle_t gen_h, gen_l;
    mcpwm_generator_config_t generator_config = { .gen_gpio_num = high_pins[i] };
    mcpwm_new_generator(operators[i], &generator_config, &gen_h);
    generator_config.gen_gpio_num = low_pins[i];
    mcpwm_new_generator(operators[i], &generator_config, &gen_l);

    // Set HIGH when timer is 0 and starting to count UP
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(gen_h, 
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));

    // Set LOW when timer hits match while counting UP
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(gen_h, 
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparators[i], MCPWM_GEN_ACTION_LOW)));

    // Set HIGH when timer hits match while counting DOWN
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(gen_h, 
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comparators[i], MCPWM_GEN_ACTION_HIGH)));

    // Dead time
    mcpwm_dead_time_config_t dt_config = {};
    dt_config.posedge_delay_ticks = DEAD_TIME_TICKS;

    // Apply config to highside
    mcpwm_generator_set_dead_time(gen_h, gen_h, &dt_config);

    // Apply config to lowside
    dt_config.negedge_delay_ticks = DEAD_TIME_TICKS;
    dt_config.flags.invert_output = true;

    // Make low side inverse of high side and apply config
    mcpwm_generator_set_dead_time(gen_h, gen_l, &dt_config);
  }

  mcpwm_timer_event_callbacks_t cbs = {
    .on_empty = on_timer,   // TEZ (count==0)
  };
  mcpwm_timer_register_event_callbacks(timer, &cbs, NULL);

  mcpwm_timer_enable(timer);
  mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Reset yup");
  generate_sine_table();
  mcpwm_gpio_init();
}

void loop()
{
  // do nothing
}
