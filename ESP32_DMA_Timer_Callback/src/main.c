#include "driver/dma.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"

#define DMA_CHANNEL 1
#define TIMER_GROUP TIMER_GROUP_0
#define TIMER_IDX 0
#define TIMER_DIVIDER 80
//#define TIMER_INTERVAL_SEC (0.0001) // 100 microseconds
#define TIMER_INTERVAL_SEC (0.1000) // 100 millisecond

static intr_handle_t s_timer_handle;

void onDMATimer() {
    // this function will be called every 100 microseconds
    printf("DMA timer callback called\n");
}

void initDMATimer() {
    // configure DMA
    dma_config_t dma_config = {};
    dma_config.channel = DMA_CHANNEL;
    dma_config.src_inc = DMA_ADDR_INC;
    dma_config.dst_inc = DMA_ADDR_INC;
    dma_config.src_endian = DMA_LITTLE_ENDIAN;
    dma_config.dst_endian = DMA_LITTLE_ENDIAN;
    dma_config.src_size = DMA_DATA_SIZE_BYTE;
    dma_config.dst_size = DMA_DATA_SIZE_BYTE;
    dma_config.src_burst = 1;
    dma_config.dst_burst = 1;
    dma_config.waiting_time = 10;
    dma_config.flags = DMA_FLAG_LOOP_TRANSFER;
    dma_config.callback = onDMATimer;
    dma_config.callback_param = 0;
    dma_driver_install(dma_config.channel, &dma_config, 0);

    // configure timer
    timer_config_t config = {};
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = true;
    timer_init(TIMER_GROUP, TIMER_IDX, &config);

    // set timer interval
    timer_set_counter_value(TIMER_GROUP, TIMER_IDX, 0x00000000ull);
    timer_set_alarm_value(TIMER_GROUP, TIMER_IDX, (TIMER_INTERVAL_SEC * TIMER_BASE_CLK) / TIMER_DIVIDER);

    // enable timer interrupt
    timer_enable_intr(TIMER_GROUP, TIMER_IDX);
    esp_intr_alloc(ETS_TIMER0_INTR_SOURCE + TIMER_GROUP * 2 + TIMER_IDX, ESP_INTR_FLAG_LEVEL1, &s_timer_handle, NULL, NULL);

    // enable timer interrupt
    timer_enable_intr(TIMER_GROUP, TIMER_IDX);
    esp_intr_alloc(ETS_TIMER0_INTR_SOURCE + TIMER_GROUP * 2 + TIMER_IDX, ESP_INTR_FLAG_LEVEL1, &s_timer_handle, NULL, NULL);

    // start DMA transfer
    dma_start(DMA_CHANNEL);

    // start timer
    timer_start(TIMER_GROUP, TIMER_IDX);
}

void app_main() {
    initDMATimer();
}
