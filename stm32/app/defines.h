/* Use detect pin */
#define FATFS_USE_DETECT_PIN            1
/* Use writeprotect pin */
#define FATFS_USE_WRITEPROTECT_PIN        1
 
/* If you want to overwrite default CD pin, then change this settings */
#define FATFS_USE_DETECT_PIN_PORT        GPIOB
#define FATFS_USE_DETECT_PIN_PIN        GPIO_PIN_6
 
/* If you want to overwrite default WP pin, then change this settings */
#define FATFS_USE_WRITEPROTECT_PIN_PORT        GPIOB
#define FATFS_USE_WRITEPROTECT_PIN_PIN        GPIO_PIN_7