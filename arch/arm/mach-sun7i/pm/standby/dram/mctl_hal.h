#ifndef   _MCTL_HAL_H
#define   _MCTL_HAL_H

extern uint32 mctl_init(void);
extern void mctl_self_refresh_entry(void);
extern void mctl_self_refresh_exit(void);
extern void mctl_power_down_entry(void);
extern void mctl_power_down_exit(void);
extern void mctl_power_save_entry(void);
extern void mctl_power_save_exit(void);

#endif  //_MCTL_HAL_H
