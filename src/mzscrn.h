#ifndef MZSCRN_H_  
#define MZSCRN_H_
	
int mz_screen_init(void);
void mz_screen_finish(void);
int font_load(const char *);

void update_scrn(void);
void updateStatusArea(const char* message);
void updateStatusSubArea(const char* message, int color);
  
void set_scren_update_valid_flag(boolean flag);

#endif

