#ifndef __KEY_CONFIG_H__
#define __KEY_CONFIG_H__

#ifdef __C51__  // 8051 编译器
#define FLASH_CODE code
#define FLASH_XDATA xdata
#else  // ARM / STM32
#define FLASH_CODE
#define FLASH_XDATA
#endif
// 按键配置 普通按键 触摸按键
// #define IS_NOT_TOUCH

/* 实现 短按 长按  长按持续触发 */
// 防抖门限：5ms
#define DEBOUNCE_THRESHOLD 10
// 长按：2秒（250ms）   0 关闭
#define LONG_PRESS_TICK 250
// 长按(连续触发)：3秒（300ms） 0 关闭
#define LONG_PRESS_AUTO_TICK 200
// 每200ms触发一次（200ms）
#define AUTO_REPEAT_INTERVAL 20
// 松开确认计数  update date 2026-1-16 Cnn
#define KEY_RELEASE_CONFIRM_CNT 5  // 5 × 10ms = 50ms

typedef struct
{
    unsigned long single;          // 按键值
    unsigned char value;           // 短按返回值
    unsigned char long_value;      // 长按返回值
    unsigned char long_flag;       // 长按标记 (1长按标记 2长按连续触发标记 0 无长按功能)
    unsigned int  key_press_tick;  // 单独设置长按时间 默认  LONG_PRESS_TICK LONG_PRESS_AUTO_TICK  update date 2025-10-29 Cnn
} KEY_Code_t;

// 按键值与功能映射表
// 每个条目包含：按键值，短按返回值，长按返回值，长按类型 单独设置长按时间
// 长按类型：0=无长按，1=单次长按，2=长按连续触发
const KEY_Code_t FLASH_CODE KEYS_LIST[] = {
    {0x0800, 1, 101, 0, 0},      // Wash 清洗
    {0x1000, 2, 103, 0, 0},      // 蒸汽
    {0x2000, 3, 105, 0, 0},      // Dry 烘敢
    {0x0400, 4, 102, 0, 0},      // 保管
    {0x0080, 5, 104, 0, 0},      // 灯
    {0x0040, 6, 106, 0, 0},      // 清洁
    {0x0020, 7, 107, 0, 0},      // 暂停继续
    {0x400000, 8, 108, 1, 150},  // 电源

    {0x0400 | 0x0020, 29, 229, 1, 0},  // 复合键  保管+暂停继续  自检

};
#define KEYS_COUNT (sizeof(KEYS_LIST) / sizeof(KEYS_LIST[0]))

#endif