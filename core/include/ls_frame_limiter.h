#ifndef LS_FRAME_LIMITER_H
#define LS_FRAME_LIMITER_H

#define LS_FPS_FULL 60.0
#define LS_FPS_ECO  30.0

#define LS_DT_MIN 0.001
#define LS_DT_MAX 0.1

typedef struct {
    double target_fps;
    double frame_duration; /* 1.0 / target_fps */
    double last_tick;      /* timestamp of previous frame start (seconds) */
    double delta_time;     /* elapsed since last frame (clamped) */
    int    eco_mode;       /* 0 = 60 fps, 1 = 30 fps */
} LSFrameLimiter;

void ls_limiter_init(LSFrameLimiter *lim, int eco_mode);
void ls_limiter_begin(LSFrameLimiter *lim);

/*
 * Computes delta_time, sleeps remainder if frame finished early.
 * Call at the end of each frame.
 */
void ls_limiter_end(LSFrameLimiter *lim);

void ls_limiter_toggle_eco(LSFrameLimiter *lim);

/* Platform time in seconds (monotonic clock). */
double ls_time_now(void);

#endif /* LS_FRAME_LIMITER_H */
