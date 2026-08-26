/* progress.h - saves/restores curriculum position across restarts.
 *
 * The device runs as a systemd service and gets restarted (crash, power
 * cycle, redeploy), so mastery progress must survive that or a child would
 * be dropped back to the start every time. The in-progress rolling mastery
 * window is deliberately NOT persisted (restarting mid-window and losing a
 * few attempts of context is an acceptable simplification); only the
 * current stage/subphase and each fact's scheduler bucket are saved.
 */
#ifndef PROGRESS_H
#define PROGRESS_H

#include "curriculum.h"

#ifndef PROGRESS_PATH
#define PROGRESS_PATH "progress.dat"
#endif

/* Returns 0 and fills in stage, subphase and sched on success, -1 if no
 * save file exists yet (caller should start fresh). */
int progress_load(const char *path, int *stage, int *subphase,
                   FactScheduler *sched);

void progress_save(const char *path, int stage, int subphase,
                    const FactScheduler *sched);

#endif
