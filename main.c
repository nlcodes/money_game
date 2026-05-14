#include "stm32_hal/hal/hal.h"
#include "bitmap.h"
#include <string.h>

#define NUM_OF_BUTTONS 16
#define MAX_COINS 4

volatile uint8_t matrix_buttons[NUM_OF_BUTTONS] = {0};

typedef struct {
  int16_t x;
  int16_t y;
  uint8_t active;
} Coin;

Coin coins[MAX_COINS];

static uint16_t rng_state = 42;

static uint16_t pseudo_rand(void) {
  rng_state ^= (rng_state << 7);
  rng_state ^= (rng_state >> 9);
  rng_state ^= (rng_state << 8);
  return rng_state;
}

static void spawn_coin(void) {
  for(int i = 0; i < MAX_COINS; i++) {
    if(!coins[i].active) {
      coins[i].x = (pseudo_rand() % 15) * 8;
      coins[i].y = 0;
      coins[i].active = 1;
      break;
    }
  }
}

static void tick_coins(uint8_t *coin_bmp, uint16_t x_counter, uint16_t y_counter, uint8_t *tick_count) {
  *tick_count += 1;
  rng_state ^= (uint16_t)(*tick_count * 31);

  if(*tick_count % 5 == 0) {
    spawn_coin();
  }

  for(int i = 0; i < MAX_COINS; i++) {
    if(!coins[i].active) continue;

    hal_clear_binary_data(coin_bmp, 8, 8, coins[i].x, coins[i].y);
    coins[i].y += 8;

    if(coins[i].y >= 64) {
      coins[i].active = 0;
      continue;
    }

    if(coins[i].x < x_counter + 8 && coins[i].x + 8 > x_counter &&
       coins[i].y < y_counter + 8 && coins[i].y + 8 > y_counter) {
      coins[i].active = 0;
      continue;
    }

    hal_draw_binary_data(coin_bmp, 8, 8, coins[i].x, coins[i].y);
  }
}

int main() {
  volatile uint8_t running = 1;

  volatile uint16_t x_counter = 64;
  volatile uint16_t y_counter = 56;
  volatile uint8_t jumping = 0;

  uint8_t tick_count = 0;

  hal_timer_interrupt_init(1000000);
  hal_display_init();
  hal_gpio_init();

  uint8_t sprite_bmp_copy[64];
  memcpy(sprite_bmp_copy, smiley_binary, 64);

  uint8_t coin_bmp[64];
  memcpy(coin_bmp, coin, 64);

  while(running) {
    hal_read_write_buttons(matrix_buttons);

    if(matrix_buttons[2]) {
      hal_clear_binary_data(sprite_bmp_copy, 8, 8, x_counter, y_counter);
      x_counter += 8;

      if(x_counter >= 120) {
        x_counter = 120;
      }
    } else if(matrix_buttons[3]) {
      hal_clear_binary_data(sprite_bmp_copy, 8, 8, x_counter, y_counter);
      x_counter -= 8;

      if(x_counter <= 8) {
        x_counter = 8;
      }
    }

    if(matrix_buttons[1] && !jumping) {
      hal_clear_binary_data(sprite_bmp_copy, 8, 8, x_counter, y_counter);
      y_counter -= 16;
      jumping = 1;
    }

    hal_draw_binary_data(sprite_bmp_copy, 8, 8, x_counter, y_counter);
    hal_buffer_write();

    while(y_counter < 56 && jumping) {
      hal_clear_binary_data(sprite_bmp_copy, 8, 8, x_counter, y_counter);
      y_counter += 8;

      hal_draw_binary_data(sprite_bmp_copy, 8, 8, x_counter, y_counter);

      tick_coins(coin_bmp, x_counter, y_counter, &tick_count);

      hal_timer_interrupt_change_delay(1000000);
      hal_timer_interrupt_reset();
      while(!hal_timer_interrupt_check());
      hal_buffer_write();

      if(y_counter >= 56) {
        jumping = 0;
      }
    }

    if(hal_timer_interrupt_check()) {
      hal_timer_interrupt_reset();

      tick_coins(coin_bmp, x_counter, y_counter, &tick_count);

      hal_buffer_write();
    }
  }

  return 0;
}
