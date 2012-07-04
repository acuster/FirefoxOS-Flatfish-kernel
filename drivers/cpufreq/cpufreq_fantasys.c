/*
 * drivers/cpufreq/cpufreq_fantasys.c
 *
 * copyright (c) 2012-2013 allwinnertech
 *
 * based on ondemand policy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/threads.h>
#include <linux/reboot.h>


/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */


#if (1)
#define FANTASY_DBG(format,args...)     printk("[fantasy] dbg "format,##args)
#define FANTASY_WRN(format,args...)     printk("[fantasy] wrn "format,##args)
#define FANTASY_ERR(format,args...)     printk("[fantasy] err "format,##args)
#else
#define FANTASY_DBG(format,args...)
#define FANTASY_WRN(format,args...)
#define FANTASY_ERR(format,args...)
#endif


/*
 * define management struct for stat thread loading
 */
struct runqueue_data {
    unsigned int rq_avg;            /* average of stat thread loading   */
    unsigned int update_rate;       /* rate of stat thread loading      */
    int64_t last_time;              /* last stat time                   */
    int64_t total_time;             /* total stat time                  */
    struct delayed_work work;       /* timer for stat loading           */
    struct workqueue_struct *rq_wq; /* workqueue for simulate timer     */
    spinlock_t lock;                /* spin lock for protect operation  */
};

#define RQ_AVG_TIMER_RATE   20      /* default state cycle is 20ms      */
static struct runqueue_data *rq_data;



/*
 * start timer for stat system thread loading
 */
static void start_rq_work(void)
{
    rq_data->rq_avg = 0;
    rq_data->last_time = 0;
    rq_data->total_time = 0;
    if (rq_data->rq_wq == NULL) {
        rq_data->rq_wq = create_singlethread_workqueue("nr_run_avg");
    }

    queue_delayed_work(rq_data->rq_wq, &rq_data->work, msecs_to_jiffies(rq_data->update_rate));
}

/*
 * stop timer
 */
static void stop_rq_work(void)
{
    if (rq_data->rq_wq) {
        cancel_delayed_work(&rq_data->work);
    }
}

/*
 * thread loading stat function
 */
static void rq_work_fn(struct work_struct *work)
{
    int64_t time_diff = 0;
    int64_t nr_run = 0;
    unsigned long flags = 0;
    int64_t cur_time = ktime_to_ns(ktime_get());

    spin_lock_irqsave(&rq_data->lock, flags);

    /* init last timer and total time */
    if (rq_data->last_time == 0)
        rq_data->last_time = cur_time;
    if (rq_data->rq_avg == 0)
        rq_data->total_time = 0;

    /* get count of current running sthread */
    nr_run = nr_running() * 100;
    FANTASY_DBG("current thread loading:%llu\n", nr_run);

    /* calculate delta time */
    time_diff = cur_time - rq_data->last_time;
    do_div(time_diff, 1000 * 1000);

    /* nr_avg =  (nr_run * △Time + nr_avg’ * totaltime’) / totaltime */
    if (time_diff != 0 && rq_data->total_time != 0) {
        nr_run = (nr_run * time_diff) + (rq_data->rq_avg * rq_data->total_time);
        do_div(nr_run, rq_data->total_time + time_diff);
    }
    rq_data->rq_avg = nr_run;
    rq_data->total_time += time_diff;
    rq_data->last_time = cur_time;

    if (rq_data->update_rate != 0) {
        queue_delayed_work(rq_data->rq_wq, &rq_data->work, msecs_to_jiffies(rq_data->update_rate));
    }

    spin_unlock_irqrestore(&rq_data->lock, flags);
}


/*
 * init thread loading stat manager
 */
static int __init init_rq_avg(void)
{
    rq_data = kzalloc(sizeof(struct runqueue_data), GFP_KERNEL);
    if (rq_data == NULL) {
        FANTASY_ERR("%s allocate memory (%d) failed! \n", __func__, sizeof(struct runqueue_data));
        return -ENOMEM;
    }
    spin_lock_init(&rq_data->lock);
    rq_data->update_rate = RQ_AVG_TIMER_RATE;
    INIT_DELAYED_WORK_DEFERRABLE(&rq_data->work, rq_work_fn);

    return 0;
}

/*
 * get runqueue average
 */
static unsigned int get_rq_avg(void)
{
    unsigned int nr_run_avg;
    unsigned long flags = 0;

    spin_lock_irqsave(&rq_data->lock, flags);
    /* get average thread loading in these stat cycles  */
    nr_run_avg = rq_data->rq_avg;
    /* reset average thread loading, start next one     */
    rq_data->rq_avg = 0;
    spin_unlock_irqrestore(&rq_data->lock, flags);

    return nr_run_avg;
}


/*
 * define thread loading policy table for cpu hotplug
 * if(rq > hotplug_rq[x][1]) cpu hotplug in;
 * if(rq < hotplug_rq[x][0]) cpu hotplug out;
 */
static int hotplug_rq[4][2] = {
    {0  , 100},
    {100, 200},
    {200, 300},
    {300, 0  }
};

/*
 * define frequncy loading policy table for cpu hotplug
 * if(freq > hotplug_freq[x][1]) cpu hotplug in;
 * if(freq < hotplug_freq[x][0]) cpu hotplug out;
 */
static int hotplug_freq[4][2] = {
    {0     , 500000},
    {200000, 500000},
    {200000, 500000},
    {200000, 0     }
};


/*
 * define data structure for dbs
 */
struct cpu_dbs_info_s {
    u64 prev_cpu_idle;          /* previous idle time statistic */
    u64 prev_cpu_iowait;        /* previous iowait time stat    */
    u64 prev_cpu_wall;          /* previous total time stat     */
    u64 prev_cpu_nice;          /* previous nice time           */
    struct cpufreq_policy *cur_policy;
    struct delayed_work work;
    struct work_struct up_work;     /* cpu plug-in processor    */
    struct work_struct down_work;   /* cpu plug-out processer   */
    struct cpufreq_frequency_table *freq_table;
    unsigned int rate_mult;
    int cpu;                    /* current cpu number           */
    /*
     * percpu mutex that serializes governor limit change with
     * do_dbs_timer invocation. We do not want do_dbs_timer to run
     * when user is changing the governor or limits.
     */
    struct mutex timer_mutex;   /* semaphore for protection     */
};

/*
 * define percpu variable for dbs information
 */
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);
struct workqueue_struct *dvfs_workqueue;
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event);

/* number of CPUs using this policy */
static unsigned int dbs_enable;

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);


#define DEF_FREQUENCY_DOWN_DIFFERENTIAL (5)
#define DEF_FREQUENCY_UP_THRESHOLD      (85)
#define DEF_CPU_UP_RATE                 (10)
#define DEF_CPU_DOWN_RATE               (20)
#define DEF_MAX_CPU_LOCK                (0)
#define DEF_START_DELAY                 (0)
#define MAX_HOTPLUG_RATE                (80u)
#define DEF_CPU_UP_RATE                 (10)
#define DEF_CPU_DOWN_RATE               (20)

#define MIN_FREQUENCY_UP_THRESHOLD      (11)
#define MAX_FREQUENCY_UP_THRESHOLD      (100)
#define DEF_SAMPLING_RATE               (50000)

static struct dbs_tuners {
    unsigned int sampling_rate;     /* dvfs sample rate         */
    unsigned int up_threshold;      /* cpu freq up threshold, up freq if higher     */
    unsigned int down_threshold;    /* cpu freq down threshold, down freq if lower  */
    unsigned int ignore_nice;       /* flag to mark if need calculate the nice time */
    unsigned int io_is_busy;        /* flag to mark if iowait calculate as cpu busy */

    /* pegasusq tuners */
    unsigned int cpu_up_rate;       /* history sample rate for cpu up               */
    unsigned int cpu_down_rate;     /* history sample rate for cpu down             */
    unsigned int max_cpu_lock;      /* max count of online cpu, user limit          */
    atomic_t hotplug_lock;          /* lock cpu online number, disable plug-in/out  */
    unsigned int dvfs_debug;        /* dvfs debug flag, print dbs information       */
} dbs_tuners_ins = {
    .up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
    .down_threshold = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
    .ignore_nice = 0,
    .cpu_up_rate = DEF_CPU_UP_RATE,
    .cpu_down_rate = DEF_CPU_DOWN_RATE,
    .max_cpu_lock = DEF_MAX_CPU_LOCK,
    .hotplug_lock = ATOMIC_INIT(0),
    .dvfs_debug = 0,
};


struct cpufreq_governor cpufreq_gov_fantasys = {
    .name                   = "fantasys",
    .governor               = cpufreq_governor_dbs,
    .owner                  = THIS_MODULE,
};


/*
 * CPU hotplug lock interface
 */
static atomic_t g_hotplug_lock = ATOMIC_INIT(0);

/*
 * apply cpu hotplug lock, up or down cpu
 */
static void apply_hotplug_lock(void)
{
    int online, possible, lock, flag;
    struct work_struct *work;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    online = num_online_cpus();
    possible = num_possible_cpus();
    lock = atomic_read(&g_hotplug_lock);
    flag = lock - online;

    if (flag == 0)
        return;

    work = flag > 0 ? &dbs_info->up_work : &dbs_info->down_work;

    FANTASY_DBG("%s online:%d possible:%d lock:%d flag:%d %d\n",
         __func__, online, possible, lock, flag, (int)abs(flag));

    queue_work_on(dbs_info->cpu, dvfs_workqueue, work);
}

/*
 * lock cpu number, the number of onlie cpu should less then num_core
 */
int cpufreq_fantasys_cpu_lock(int num_core)
{
    int prev_lock;

    if (num_core < 1 || num_core > num_possible_cpus())
        return -EINVAL;

    prev_lock = atomic_read(&g_hotplug_lock);
    if (prev_lock != 0 && prev_lock < num_core)
        return -EINVAL;

    atomic_set(&g_hotplug_lock, num_core);
    apply_hotplug_lock();

    return 0;
}

/*
 * unlock cpu hotplug number
 */
int cpufreq_fantasys_cpu_unlock(int num_core)
{
    int prev_lock = atomic_read(&g_hotplug_lock);

    if(prev_lock != num_core)
        return -EINVAL;

    atomic_set(&g_hotplug_lock, 0);

    return 0;
}

/*
 * history of CPU usage
 */
struct cpu_usage {
    unsigned int freq;          /* cpu frequency value      */
    unsigned int load[NR_CPUS]; /* cpu frequency loading    */
    unsigned int rq_avg;        /* average thread loading   */
};
/* record cpu history loading information */
struct cpu_usage_history {
    struct cpu_usage usage[MAX_HOTPLUG_RATE];
    unsigned int num_hist;
};
static struct cpu_usage_history *hotplug_history;


/* cpufreq_pegasusq Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(up_threshold, up_threshold);
show_one(down_threshold, down_threshold);
show_one(cpu_up_rate, cpu_up_rate);
show_one(cpu_down_rate, cpu_down_rate);
show_one(max_cpu_lock, max_cpu_lock);
show_one(dvfs_debug, dvfs_debug);
static ssize_t show_hotplug_lock(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&g_hotplug_lock));
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_rate = input;
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
	    input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold = input;
	return count;
}

static ssize_t store_down_threshold(struct kobject *a, struct attribute *b,
				       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.down_threshold = min(input, 100u);
	return count;
}

static ssize_t store_cpu_up_rate(struct kobject *a, struct attribute *b,
				 const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.cpu_up_rate = min(input, MAX_HOTPLUG_RATE);
	return count;
}

static ssize_t store_cpu_down_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.cpu_down_rate = min(input, MAX_HOTPLUG_RATE);
	return count;
}

static ssize_t store_max_cpu_lock(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.max_cpu_lock = min(input, num_possible_cpus());
	return count;
}

static ssize_t store_hotplug_lock(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int prev_lock;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	input = min(input, num_possible_cpus());
	prev_lock = atomic_read(&dbs_tuners_ins.hotplug_lock);

	if (prev_lock)
		cpufreq_fantasys_cpu_unlock(prev_lock);

	if (input == 0) {
		atomic_set(&dbs_tuners_ins.hotplug_lock, 0);
		return count;
	}

	ret = cpufreq_fantasys_cpu_lock(input);
	if (ret) {
		printk(KERN_ERR "[HOTPLUG] already locked with smaller value %d < %d\n",
			atomic_read(&g_hotplug_lock), input);
		return ret;
	}

	atomic_set(&dbs_tuners_ins.hotplug_lock, input);

	return count;
}

static ssize_t store_dvfs_debug(struct kobject *a, struct attribute *b,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.dvfs_debug = input > 0;
	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(up_threshold);
define_one_global_rw(down_threshold);
define_one_global_rw(cpu_up_rate);
define_one_global_rw(cpu_down_rate);
define_one_global_rw(max_cpu_lock);
define_one_global_rw(hotplug_lock);
define_one_global_rw(dvfs_debug);

static struct attribute *dbs_attributes[] = {
	&sampling_rate.attr,
	&up_threshold.attr,
	&io_is_busy.attr,
	&down_threshold.attr,
	&cpu_up_rate.attr,
	&cpu_down_rate.attr,
	&max_cpu_lock.attr,
	&hotplug_lock.attr,
	&dvfs_debug.attr,
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "fantasys",
};


/*
 * cpu hotplug, just plug in one cpu
 */
static void cpu_up_work(struct work_struct *work)
{
    int cpu, nr_up;
    int online = num_online_cpus();
    int hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_up = (hotplug_lock - online) > 0? (hotplug_lock-online) : 0;
    } else {
        nr_up = 1;
    }

    for_each_cpu_not(cpu, cpu_online_mask) {
        if (nr_up-- == 0)
            break;
        if (cpu == 0)
            continue;
        FANTASY_DBG("cpu up:%d\n", cpu);
        cpu_up(cpu);
    }
}

/*
 * cpu hotplug, cpu plugout
 */
static void cpu_down_work(struct work_struct *work)
{
    int cpu;
    int online = num_online_cpus();
    int nr_down;
    int hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_down = (online - hotplug_lock) > 0? (online-hotplug_lock) : 0;
    } else {
        nr_down = 1;
    }

    for_each_online_cpu(cpu) {
        if (nr_down-- == 0)
            break;

        if (cpu == 0)
            continue;
        printk("cpu down:%d\n", cpu);
        cpu_down(cpu);
    }
}

/*
 * print hotplug debugging info.
 * which 1 : up, 0 : down
 */
static void debug_hotplug_check(int which, int rq_avg, int freq,
             struct cpu_usage *usage)
{
    int cpu;

    printk("check %s rq %d.%02d freq %d [", which ? "up" : "down",
           rq_avg / 100, rq_avg % 100, freq);
    for_each_online_cpu(cpu) {
        printk("(%d, %d), ", cpu, usage->load[cpu]);
    }
    printk(KERN_ERR "]\n");
}

/*
 * check if need plug in one cpu core
 */
static int check_up(void)
{
    int num_hist = hotplug_history->num_hist;
    struct cpu_usage *usage;
    int freq, rq_avg;
    int i;
    int up_rate = dbs_tuners_ins.cpu_up_rate;
    int up_freq, up_rq;
    int min_freq = INT_MAX;
    int min_rq_avg = INT_MAX;
    int online;
    int hotplug_lock = atomic_read(&g_hotplug_lock);

    /* hotplug has been locked, do nothing */
    if (hotplug_lock > 0)
        return 0;

    online = num_online_cpus();
    up_freq = hotplug_freq[online-1][1];
    up_rq = hotplug_rq[online-1][1];

    /* check if count of the cpu reached the max value */
    if (online == num_possible_cpus())
        return 0;
    if (dbs_tuners_ins.max_cpu_lock != 0 && online >= dbs_tuners_ins.max_cpu_lock)
        return 0;

    /* check if reached the switch point */
    if (num_hist == 0 || num_hist % up_rate)
        return 0;

    for (i = num_hist - 1; i >= num_hist - up_rate; --i) {
        usage = &hotplug_history->usage[i];

        freq = usage->freq;
        rq_avg =  usage->rq_avg;

        min_freq = min(min_freq, freq);
        min_rq_avg = min(min_rq_avg, rq_avg);

        if (dbs_tuners_ins.dvfs_debug)
            debug_hotplug_check(1, rq_avg, freq, usage);
    }

    if (min_freq >= up_freq && min_rq_avg > up_rq) {
        FANTASY_DBG("cpu need plugin, freq: %d>=%d && rq: %d>%d\n", min_freq, up_freq, min_rq_avg, up_rq);
        /* need plug in cpu, reset the stat cycle */
        hotplug_history->num_hist = 0;
        return 1;
    }
    return 0;
}

/*
 * check if need plug out one cpu core
 */
static int check_down(void)
{
    int num_hist = hotplug_history->num_hist;
    struct cpu_usage *usage;
    int freq, rq_avg;
    int i;
    int down_rate = dbs_tuners_ins.cpu_down_rate;
    int down_freq, down_rq;
    int max_freq = 0;
    int max_rq_avg = 0;
    int online;
    int hotplug_lock = atomic_read(&g_hotplug_lock);

    /* hotplug has been locked, do nothing */
    if (hotplug_lock > 0)
        return 0;

    online = num_online_cpus();
    down_freq = hotplug_freq[online-1][0];
    down_rq = hotplug_rq[online-1][0];

    /* just one cpu, can't be plug out */
    if (online == 1)
        return 0;

    /* if count of online cpu is larger than the max value, plug out cpu */
    if (dbs_tuners_ins.max_cpu_lock != 0 && online > dbs_tuners_ins.max_cpu_lock)
        return 1;

    /* check if reached the switch point */
    if (num_hist == 0 || num_hist % down_rate)
        return 0;

    for (i = num_hist - 1; i >= num_hist - down_rate; --i) {
        usage = &hotplug_history->usage[i];

        freq = usage->freq;
        rq_avg = usage->rq_avg;

        max_freq = max(max_freq, freq);
        max_rq_avg = max(max_rq_avg, rq_avg);

        if (dbs_tuners_ins.dvfs_debug)
            debug_hotplug_check(0, rq_avg, freq, usage);
    }

    if (max_freq <= down_freq && max_rq_avg <= down_rq) {
        FANTASY_DBG("cpu need plugout, freq: %d<=%d && rq: %d<%d\n", max_freq, down_freq, max_rq_avg, down_rq);
        /* need plug in cpu, reset the stat cycle */
        hotplug_history->num_hist = 0;
        return 1;
    }

    return 0;
}


static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

/*
 * check if need plug in/out cpu, if need increase/decrease cpu frequency
 */
static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
    unsigned int max_load_freq;

    struct cpufreq_policy *policy;
    unsigned int j;
    int num_hist = hotplug_history->num_hist;
    int max_hotplug_rate = max(dbs_tuners_ins.cpu_up_rate, dbs_tuners_ins.cpu_down_rate);

    policy = this_dbs_info->cur_policy;

    hotplug_history->usage[num_hist].freq = policy->cur;
    hotplug_history->usage[num_hist].rq_avg = get_rq_avg();
    ++hotplug_history->num_hist;

    /* Get Absolute Load - in terms of freq */
    max_load_freq = 0;

    for_each_cpu(j, policy->cpus) {
        struct cpu_dbs_info_s *j_dbs_info;
        u64 cur_wall_time, cur_idle_time, cur_iowait_time;
        u64 prev_wall_time, prev_idle_time, prev_iowait_time;
        unsigned int idle_time, wall_time, iowait_time;
        unsigned int load, load_freq;
        int freq_avg;

        j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
        prev_wall_time = j_dbs_info->prev_cpu_wall;
        prev_idle_time = j_dbs_info->prev_cpu_idle;
        prev_iowait_time = j_dbs_info->prev_cpu_iowait;

        cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
        cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

        wall_time = cur_wall_time - prev_wall_time;
        j_dbs_info->prev_cpu_wall = cur_wall_time;

        idle_time = cur_idle_time - prev_idle_time;
        j_dbs_info->prev_cpu_idle = cur_idle_time;

        iowait_time = cur_iowait_time - prev_iowait_time;
        j_dbs_info->prev_cpu_iowait = cur_iowait_time;


        if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
            idle_time -= iowait_time;

        if (unlikely(!wall_time || wall_time < idle_time))
            continue;

        load = 100 * (wall_time - idle_time) / wall_time;
        hotplug_history->usage[num_hist].load[j] = load;

        freq_avg = __cpufreq_driver_getavg(policy, j);
        if (freq_avg <= 0)
            freq_avg = policy->cur;

        load_freq = load * freq_avg;
        if (load_freq > max_load_freq)
            max_load_freq = load_freq;
    }

    /* Check for CPU hotplug */
    if (check_up()) {
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->up_work);
    } else if (check_down()) {
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->down_work);
    }

    /* check if history array is out of range */
    if (hotplug_history->num_hist == max_hotplug_rate)
        hotplug_history->num_hist = 0;
}


static void do_dbs_timer(struct work_struct *work)
{
    struct cpu_dbs_info_s *dbs_info = container_of(work, struct cpu_dbs_info_s, work.work);
    unsigned int cpu = dbs_info->cpu;
    int delay;

    mutex_lock(&dbs_info->timer_mutex);

    dbs_check_cpu(dbs_info);

	/* We want all CPUs to do sampling nearly on
	 * same jiffy
	 */
	delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

    queue_delayed_work_on(cpu, dvfs_workqueue, &dbs_info->work, delay);
    mutex_unlock(&dbs_info->timer_mutex);
}


static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(DEF_START_DELAY * 1000 * 1000 + dbs_tuners_ins.sampling_rate);
	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	INIT_WORK(&dbs_info->up_work, cpu_up_work);
	INIT_WORK(&dbs_info->down_work, cpu_down_work);

	queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work, delay + 2 * HZ);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
	cancel_work_sync(&dbs_info->up_work);
	cancel_work_sync(&dbs_info->down_work);
}

static int reboot_notifier_call(struct notifier_block *this,
                unsigned long code, void *_cmd)
{
    atomic_set(&g_hotplug_lock, 1);
    return NOTIFY_DONE;
}
static struct notifier_block reboot_notifier = {
    .notifier_call = reboot_notifier_call,
};

/*
 * cpufreq dbs governor
 */
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event)
{
    unsigned int cpu = policy->cpu;
    struct cpu_dbs_info_s *this_dbs_info;
    unsigned int j;
    int rc;

    this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

    switch (event) {
        case CPUFREQ_GOV_START:
        {
            if ((!cpu_online(cpu)) || (!policy->cur))
                return -EINVAL;

            hotplug_history->num_hist = 0;
            /* start run queue loading stat work */
            start_rq_work();

            mutex_lock(&dbs_mutex);

            dbs_enable++;
            for_each_cpu(j, policy->cpus) {
                struct cpu_dbs_info_s *j_dbs_info;
                j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
                j_dbs_info->cur_policy = policy;

                j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j, &j_dbs_info->prev_cpu_wall);
            }
            this_dbs_info->cpu = cpu;

            /*
             * Start the timerschedule work, when this governor
             * is used for first time
             */
            if (dbs_enable == 1) {
                rc = sysfs_create_group(cpufreq_global_kobject, &dbs_attr_group);
                if (rc) {
                    mutex_unlock(&dbs_mutex);
                    return rc;
                }

                dbs_tuners_ins.sampling_rate = DEF_SAMPLING_RATE;
                dbs_tuners_ins.io_is_busy = 0;
            }
            mutex_unlock(&dbs_mutex);

            /* register reboot notifier for process cpus when reboot */
            register_reboot_notifier(&reboot_notifier);

            mutex_init(&this_dbs_info->timer_mutex);
            dbs_timer_init(this_dbs_info);

            break;
        }

        case CPUFREQ_GOV_STOP:
        {
            dbs_timer_exit(this_dbs_info);

            mutex_lock(&dbs_mutex);
            mutex_destroy(&this_dbs_info->timer_mutex);

            unregister_reboot_notifier(&reboot_notifier);

            dbs_enable--;
            mutex_unlock(&dbs_mutex);

            stop_rq_work();

            if (!dbs_enable)
                sysfs_remove_group(cpufreq_global_kobject, &dbs_attr_group);

            break;
        }

        case CPUFREQ_GOV_LIMITS:
        {
            mutex_lock(&this_dbs_info->timer_mutex);

            if (policy->max < this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->max, CPUFREQ_RELATION_H);
            else if (policy->min > this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->min, CPUFREQ_RELATION_L);

            mutex_unlock(&this_dbs_info->timer_mutex);
            break;
        }
    }
    return 0;
}


/*
 * cpufreq governor dbs initiate
 */
static int __init cpufreq_gov_dbs_init(void)
{
    int ret;

    ret = init_rq_avg();
    if (ret)
        return ret;

    hotplug_history = kzalloc(sizeof(struct cpu_usage_history), GFP_KERNEL);
    if (!hotplug_history) {
        FANTASY_ERR("%s cannot create hotplug history array\n", __func__);
        ret = -ENOMEM;
        goto err_hist;
    }

    /* create dvfs daemon */
    dvfs_workqueue = create_workqueue("fantasys");
    if (!dvfs_workqueue) {
        pr_err("%s cannot create workqueue\n", __func__);
        ret = -ENOMEM;
        goto err_queue;
    }

    /* register cpu freq governor */
    ret = cpufreq_register_governor(&cpufreq_gov_fantasys);
    if (ret)
        goto err_reg;

    return ret;

err_reg:
    destroy_workqueue(dvfs_workqueue);
err_queue:
    kfree(hotplug_history);
err_hist:
    kfree(rq_data);
    return ret;
}

/*
 * cpufreq governor dbs exit
 */
static void __exit cpufreq_gov_dbs_exit(void)
{
    cpufreq_unregister_governor(&cpufreq_gov_fantasys);
    destroy_workqueue(dvfs_workqueue);
    kfree(hotplug_history);
    kfree(rq_data);
}

MODULE_AUTHOR("kevin.z.m <kevin@allwinnertech.com>");
MODULE_DESCRIPTION("'cpufreq_fantasys' - A dynamic cpufreq/cpuhotplug governor");
MODULE_LICENSE("GPL");

module_init(cpufreq_gov_dbs_init);
module_exit(cpufreq_gov_dbs_exit);
