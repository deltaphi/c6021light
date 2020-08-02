#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


#define configUSE_PREEMPTION      1
#define configCPU_CLOCK_HZ        ( ( unsigned long ) 72000000 )
#define configSYSTICK_CLOCK_HZ    ( configCPU_CLOCK_HZ / 8 )
#define configTICK_RATE_HZ        ( ( TickType_t ) 250 )
#define configMAX_PRIORITIES      ( 5 )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY      ( 5 )
#define configMINIMAL_STACK_SIZE  ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE     ( ( size_t ) ( 5 * 1024 ) )
#define configMAX_TASK_NAME_LEN   ( 16 )
#define configUSE_TRACE_FACILITY  0
#define configUSE_16_BIT_TICKS    0
#define configIDLE_SHOULD_YIELD   1
#define configUSE_MUTEXES         0
#define configCHECK_FOR_STACK_OVERFLOW 0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

#define INCLUDE_vTaskPrioritySet        0
#define INCLUDE_uxTaskPriorityGet       0
#define INCLUDE_vTaskDelete             0
#define INCLUDE_vTaskCleanUpResources   0
#define INCLUDE_vTaskSuspend            0
#define INCLUDE_vTaskDelayUntil         0
#define INCLUDE_vTaskDelay              1


#endif  // FREERTOS_CONFIG_H