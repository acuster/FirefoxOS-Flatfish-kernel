obj-$(CONFIG_LYCHEE_LCD_SUN7I) += lcd.o

ifeq ($(CONFIG_LYCHEE_LCD_AINOL_AUROTA),y)
lcd-objs := dev_lcd.o lcd_bak/ainol_aurota.o lcd1_panel_cfg.o
else ifeq ($(CONFIG_LYCHEE_LCD_DEFAULT),y)
lcd-objs := dev_lcd.o lcd0_panel_cfg.o lcd1_panel_cfg.o
endif