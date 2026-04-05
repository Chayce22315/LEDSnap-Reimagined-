#include "ls_frame_limiter.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
  #include <unistd.h>
#endif

double ls_time_now(void) {
#ifdef _WIN32
    static double freq_inv = 0.0;
    if (freq_inv == 0.0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        freq_inv = 1.0 / (double)freq.QuadPart;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * freq_inv;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

static void sleep_seconds(double seconds) {
    if (seconds <= 0.0) return;
#ifdef _WIN32
    Sleep((DWORD)(seconds * 1000.0));
#else
    struct timespec req;
    req.tv_sec  = (time_t)seconds;
    req.tv_nsec = (long)((seconds - (double)req.tv_sec) * 1e9);
    nanosleep(&req, NULL);
#endif
}

void ls_limiter_init(LSFrameLimiter *lim, int eco_mode) {
    lim->eco_mode       = eco_mode;
    lim->target_fps     = eco_mode ? LS_FPS_ECO : LS_FPS_FULL;
    lim->frame_duration = 1.0 / lim->target_fps;
    lim->last_tick      = ls_time_now();
    lim->delta_time     = lim->frame_duration;
}

void ls_limiter_begin(LSFrameLimiter *lim) {
    lim->last_tick = ls_time_now();
}

void ls_limiter_end(LSFrameLimiter *lim) {
    double now = ls_time_now();
    lim->delta_time = now - lim->last_tick;

    /* clamp to prevent physics explosions or zero-dt division */
    if (lim->delta_time < LS_DT_MIN) lim->delta_time = LS_DT_MIN;
    if (lim->delta_time > LS_DT_MAX) lim->delta_time = LS_DT_MAX;

    /* sleep remaining budget to hit target framerate */
    double remaining = lim->frame_duration - (now - lim->last_tick);
    if (remaining > 0.0)
        sleep_seconds(remaining);

    lim->last_tick = ls_time_now();
}

void ls_limiter_toggle_eco(LSFrameLimiter *lim) {
    lim->eco_mode = !lim->eco_mode;
    lim->target_fps     = lim->eco_mode ? LS_FPS_ECO : LS_FPS_FULL;
    lim->frame_duration = 1.0 / lim->target_fps;
}
