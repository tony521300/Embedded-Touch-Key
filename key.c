/**
 * @file key.c
 * @brief 按键处理模块，实现按键检测、防抖、长按和连发等功能
 *
 * @details
 * 该模块主要功能：
 * - 按键状态检测和防抖处理
 * - 支持短按和长按功能
 * - 支持按键自动重复(连发)功能
 * - 按键状态机管理
 *
 * @author Cnn
 * @date 2025-09-05
 * @version 1.0
 * updtea date 2025-10-29 Cnn
 * 增加每个按键长按时间自定义
 *
 *
 * @copyright Copyright (c) 2025
 *
 * @note
 * - 支持多种按键类型配置
 * - 可通过key_config.h配置按键参数
 * - 采用状态机方式处理按键逻辑
 */
#include "key.h"

#include "key_config.h"

typedef enum
{
    KEY_STATE_IDLE = 0,  // 空闲状态，无按键按下
    KEY_STATE_DEBOUNCE,  // 防抖状态
    KEY_STATE_PRESSED,   // 按键确认按下状态
    KEY_STATE_HOLD       // 按键长按保持状态
} KeyState_t;

// 静态变量定义
static FLASH_XDATA KeyState_t    key_state = KEY_STATE_IDLE;  // 当前按键状态
static FLASH_XDATA unsigned long last_key  = 0;               // 上一次检测到的按键值

/**
 * @brief 根据按键码获取对应的按键值
 * @param last_key 按键码
 * @param is_long 是否为长按事件（非0表示长按，0表示短按）
 * @return 对应的按键值，如果未找到则返回0
 */
static unsigned char key_value_get(unsigned long last_key, unsigned char is_long)
{
    unsigned char i;
    for (i = 0; i < KEYS_COUNT; i++)
    {
        if (KEYS_LIST[i].single == last_key)
        {
            return is_long ? KEYS_LIST[i].long_value : KEYS_LIST[i].value;
        }
    }
    return 0;  // 未找到对应按键，返回0
}

/**
 * @brief 判断按键是否支持长按功能
 * @param last_key 按键码
 * @param long_flag 长按类型 (1:普通长按 2:自动重复长按 0:无长按功能)
 * @return 1:支持对应类型的长按 0:不支持
 */
static unsigned char in_long_keys_table(unsigned long last_key, unsigned char long_flag)
{
    unsigned char i;
    for (i = 0; i < KEYS_COUNT; i++)
    {
        // 判断普通长按
        if (long_flag == 1 && KEYS_LIST[i].long_flag == 1 && last_key == KEYS_LIST[i].single)
        {
            return 1;
        }
        // 判断自动重复长按
        else if (long_flag == 2 && KEYS_LIST[i].long_flag == 2 && last_key == KEYS_LIST[i].single)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief 判断按键设置长按时间 默认  LONG_PRESS_TICK
 * @param last_key 按键码
 * @param long_flag 长按类型 (1:普通长按 2:自动重复长按 0:无长按功能)
 * @return 返回长按时间
 */
static unsigned int keys_press_tick(unsigned long last_key, unsigned char long_flag)
{
    unsigned char i;
    for (i = 0; i < KEYS_COUNT; i++)
    {
        // 判断普通长按
        if (KEYS_LIST[i].key_press_tick && KEYS_LIST[i].long_flag && last_key == KEYS_LIST[i].single)
        {
            return KEYS_LIST[i].key_press_tick;
        }
    }
    return long_flag == 1 ? LONG_PRESS_TICK : LONG_PRESS_AUTO_TICK;
}

/**
 * @brief 处理按键状态机，返回当前有效按键值
 * @return
 *         1~6: 各个短按按键
 *         105: 电源键长按
 *         101: 加键长按后自动触发
 *         102: 减键长按后自动触发
 *         0: 无有效按键
 */

/**
 * @brief   按键处理函数
 * @param   single  当前扫描到的按键值（0 表示无按键，非0表示有效键）
 * @return  按键事件值（短按、长按或自动连发等）
 */
static unsigned char process_key(unsigned long single)
{
    FLASH_XDATA unsigned char        key_value       = 0;  // 默认返回无事件
    static FLASH_XDATA unsigned int  press_count     = 0;  // 按键持续计数（用于防抖和长按判断）
    static FLASH_XDATA unsigned char long_press_flag = 0;  // 长按标志位
    static FLASH_XDATA unsigned int  repeat_tick     = 0;  // 自动加减重复触发计数器（用于长按连续触发）
    static FLASH_XDATA unsigned char key_done        = 0;  // 按键处理完成标志清零

    static FLASH_XDATA unsigned char release_cnt = 0;  // 松开计数器

    if (single == 0)  // ================== 按键松开 ==================
    {
        /* ================= 松开大于50ms 才算真正按键触发 start update date 2026-1-16 Cnn  ============*/
        /* 还没按过，不处理 */
        if (key_state == KEY_STATE_IDLE)
            return 0;

        /* 松开计数 用于EFT 测试 按键误触发*/
        if (release_cnt < KEY_RELEASE_CONFIRM_CNT)
        {
            release_cnt++;
            return 0;  // 未确认松开
        }
        /* ================= 松开大于50ms 才算真正按键触发 end  ============*/

        // 如果之前处于按下状态，说明这是一次完整的短按
        if (key_state == KEY_STATE_PRESSED)
        {
            key_value = key_value_get(last_key, 0);  // 返回短按键值
        }

        // 按键状态完全复位
        key_state       = KEY_STATE_IDLE;  // 回到空闲
        last_key        = 0;               // 清除上次键值
        press_count     = 0;               // 按下计数清零
        long_press_flag = 0;               // 长按标志清零
        repeat_tick     = 0;               // 自动连发计数清零
        key_done        = 0;               // 按键处理完成标志清零
        return key_value;                  // 返回事件（可能是短按）
    }
    else
    {
        release_cnt = 0;  // 按键未松开，松开计数清零
    }

    // ================== 新键按下，进入防抖阶段 ==================
    if (single != last_key)
    {
        key_state       = KEY_STATE_DEBOUNCE;  // 切换到防抖状态
        press_count     = 0;                   // 重新计数
        long_press_flag = 0;                   // 长按标志清零
        repeat_tick     = 0;                   // 连发计数清零
        last_key        = single;              // 记录新的键值
        key_done        = 0;
        return key_value;  // 暂不返回事件
    }

    // ================== 持续按下同一个键 ==================
    switch (key_state)
    {
        case KEY_STATE_DEBOUNCE:  // ---------- 防抖阶段 ----------
            if (press_count < 0xFFFF)
                press_count++;

            // 防抖时间到，确认按键有效
            if (press_count >= DEBOUNCE_THRESHOLD)
            {
                key_state   = KEY_STATE_PRESSED;  // 切换到按下状态
                press_count = 0;                  // 重新计数
            }
            break;

        case KEY_STATE_PRESSED:  // ---------- 确认按下状态 ----------
            if (press_count < 0xFFFF)
                press_count++;

            if (!long_press_flag)  // 还没有进入长按状态
            {
                // ========== 普通长按（一次性触发） ==========
                if (LONG_PRESS_TICK && in_long_keys_table(last_key, 1) && press_count >= keys_press_tick(last_key, 1))
                {
                    key_value       = key_value_get(last_key, 1);  // 返回长按事件
                    key_done        = 1;
                    long_press_flag = 1;               // 标记已长按
                    key_state       = KEY_STATE_HOLD;  // 转入保持阶段
                }
                // ========== 自动连发长按 ==========
                else if (LONG_PRESS_AUTO_TICK && in_long_keys_table(last_key, 2) && press_count >= keys_press_tick(last_key, 2))
                {
                    long_press_flag = 1;               // 进入长按模式
                    key_state       = KEY_STATE_HOLD;  // 转入保持阶段
                    repeat_tick     = 0;               // 连发计数清零
                }
            }
            break;

        case KEY_STATE_HOLD:  // ---------- 长按保持阶段 ----------
            // 自动连发功能：每隔固定时间触发一次
            if (LONG_PRESS_AUTO_TICK && in_long_keys_table(last_key, 2))
            {
                repeat_tick++;
                if (repeat_tick >= AUTO_REPEAT_INTERVAL)
                {
                    repeat_tick = 0;
                    key_value   = key_value_get(last_key, 1);  // 返回长按连发事件
                    key_done    = 1;
                }
            }
            break;

        default:
            break;
    }

    return key_value;  // 返回按键事件（无事件时返回0）
}
/**
 * @brief 获取当前按键值
 */
unsigned char key_get(unsigned long single)
{
    return process_key(single);
}