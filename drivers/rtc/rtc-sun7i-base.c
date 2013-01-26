/*
 * linux-3.3\drivers\rtc\rtc-sun7i-base.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * sunxi rtc driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

static int losc_err_flag = 0;

#define IS_LEAP_YEAR(year) (((year)%400)==0 || (((year)%100)!=0 && ((year)%4)==0))

/* get rtc time */
static int sunxi_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int have_retried = 0;

	/* only for alarm losc err occur */
	if(losc_err_flag) {
		rtc_tm->tm_sec  = 0;
		rtc_tm->tm_min  = 0;
		rtc_tm->tm_hour = 0;
		rtc_tm->tm_mday = 0;
		rtc_tm->tm_mon  = 0;
		rtc_tm->tm_year = 0;
		pr_err("%s(%d): err, losc_err_flag is 1\n", __func__, __LINE__);
		return -1;
	}

retry_get_time:
	pr_info("%s, line %d\n", __func__, __LINE__);
	/* first to get the date, then time, because the sec turn to 0 will effect the date */
	rtc_tm->tm_sec  = rtc_reg_list->r_hh_mm_ss.second;
	rtc_tm->tm_min  = rtc_reg_list->r_hh_mm_ss.minute;
	rtc_tm->tm_hour = rtc_reg_list->r_hh_mm_ss.hour;

	rtc_tm->tm_mday = rtc_reg_list->r_yy_mm_dd.day;
	rtc_tm->tm_mon  = rtc_reg_list->r_yy_mm_dd.month;
	rtc_tm->tm_year = rtc_reg_list->r_yy_mm_dd.year;

	/* the only way to work out wether the system was mid-update
	 * when we read it is to check the second counter, and if it
	 * is zero, then we re-try the entire read
	 */
	if (rtc_tm->tm_sec == 0 && !have_retried) {
		have_retried = 1;
		goto retry_get_time;
	}
	pr_info("%s(%d): rtc_tm->tm_year:%d\n", __func__, __LINE__, rtc_tm->tm_year);
	if (rtc_tm->tm_year < 70) {
		pr_info("%s(%d): year < 70, so add 100, to check\n", __func__, __LINE__);
		rtc_tm->tm_year += 100;	/* assume we are in 1970...2069 */
	}

	rtc_tm->tm_mon -= 1;
	pr_info("%s(%d): read time %d-%d-%d %d:%d:%d\n", __func__, __LINE__, rtc_tm->tm_year + 1900,
		rtc_tm->tm_mon + 1, rtc_tm->tm_mday, rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);
	return 0;
}

/* set rtc time */
static int sunxi_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	unsigned int timeout = 0;
	int actual_year = 0;
	int line = 0;

	pr_info("%s(%d): time to set %d-%d-%d %d:%d:%d\n", __func__, __LINE__, tm->tm_year + 1900,
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* int tm_year; years from 1900
	 * int tm_mon; months since january 0-11
	 * the input para tm->tm_year is the offset related 1900;
	 */
	actual_year = tm->tm_year + 1900;
	if(actual_year > 2100 || actual_year < 1970) {
		dev_err(dev, "%s(%d) err: rtc only supports 130(1970~2100) years\n", __func__, __LINE__);
		return -EINVAL;
	}

	/* Any bit of [9:7] is set, the time and date
	 * register can`t be written, we re-try the entried read
	 * check at most 3 times
	 */
	timeout = 3;
	do {
		if(rtc_reg_list->losc_ctl.rtc_yymmdd_acce
			|| rtc_reg_list->losc_ctl.rtc_hhmmss_acce
			|| rtc_reg_list->losc_ctl.alm_ddhhmmss_acce) {
			pr_err("%s(%d): canot change rtc now!\n", __func__, __LINE__);
			msleep(500);
		}
	}while(--timeout);

	/* prevent the application seting the error time */
	tm->tm_mon  += 1;
	switch(tm->tm_mon) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		if(tm->tm_mday > 31)
			line = __LINE__;
		if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
			line = __LINE__;
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		if(tm->tm_mday > 30)
			line = __LINE__;
		if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
			line = __LINE__;
		break;
	case 2:
		if(IS_LEAP_YEAR(actual_year)) {
			if(tm->tm_mday > 29)
				line = __LINE__;
			if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
				line = __LINE__;
		} else {
			if(tm->tm_mday > 28)
				line = __LINE__;
			if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
				line = __LINE__;
		}
		break;
	default:
		line = __LINE__;
		break;
	}
	if(0 != line) {
		pr_info("%s(%d) err: date %d-%d-%d %d:%d:%d, so reset to 1900-1-1 00:00:00\n", __func__, line,
			tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		tm->tm_sec  = 0;
		tm->tm_min  = 0;
		tm->tm_hour = 0;
		tm->tm_mday = 0;
		tm->tm_mon  = 0;
		tm->tm_year = 0;
	}

	pr_info("%s(%d): actually set time to %d-%d-%d %d:%d:%d\n", __func__, __LINE__, tm->tm_year + 1900,
		tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* set hour, minute, second */
	writel(0, &rtc_reg_list->r_hh_mm_ss); /* note: must add, or set date maybe err,liugang */
	rtc_reg_list->r_hh_mm_ss.hour = tm->tm_hour;
	rtc_reg_list->r_hh_mm_ss.minute = tm->tm_min;
	rtc_reg_list->r_hh_mm_ss.second = tm->tm_sec;
	timeout = 0xffff;
	while(rtc_reg_list->losc_ctl.rtc_hhmmss_acce && (--timeout));
	if(timeout == 0) {
		dev_err(dev, "%s(%d): fail to set rtc time\n", __func__, __LINE__);
		return -1;
	}

	/* set year, month, day */
	writel(0, &rtc_reg_list->r_yy_mm_dd);
	rtc_reg_list->r_yy_mm_dd.year = tm->tm_year;
	rtc_reg_list->r_yy_mm_dd.month = tm->tm_mon;
	rtc_reg_list->r_yy_mm_dd.day = tm->tm_mday;
	/* set leap year bit */
	if(IS_LEAP_YEAR(actual_year))
		rtc_reg_list->r_yy_mm_dd.leap = 1;
	timeout = 0xffff;
	while(rtc_reg_list->losc_ctl.rtc_yymmdd_acce && (--timeout));
	if(timeout == 0) {
		dev_err(dev, "%s(%d): fail to set rtc date\n", __func__, __LINE__);
		return -1;
	}

	/* wait about 70us to make sure the the time is really written into target */
	udelay(70);

	pr_info("%s(%d): end\n", __func__, __LINE__);
	return 0;
}

/* update control registers, asynchronous interrupt enable*/
static void sunxi_rtc_setaie(int to)
{
	pr_info("%s(%d): para %d\n", __func__, __LINE__, to);
	switch(to){
	case 1:
		/* enable alarm cnt */
		rtc_reg_list->a_enable.alm_cnt_en = 1;
		/* enable alarm cnt irq */
		pr_info("%s(%d): to check if need enable alarm cnt irq\n", __func__, __LINE__);
		rtc_reg_list->a_irq_enable.alarm_cnt_irq_en = 1; /* liugang add */
		break;
	case 0:
	default:
		/* disable all alarm(alarm week0~6 and alarm count) */
		writel(0, &rtc_reg_list->a_enable);
		/* disable alarm week and count irq */
		writel(0, &rtc_reg_list->a_irq_enable);
		/* clear pending count alarm */
		writel(3, &rtc_reg_list->a_irq_status);
		break;
	}
}

static int sunxi_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *alm_tm = &alrm->time;

	alm_tm->tm_sec  = rtc_reg_list->a_cnt_dd_hh_mm_ss.second;
	alm_tm->tm_min  = rtc_reg_list->a_cnt_dd_hh_mm_ss.minute;
	alm_tm->tm_hour = rtc_reg_list->a_cnt_dd_hh_mm_ss.hour;
	alm_tm->tm_mday = rtc_reg_list->r_yy_mm_dd.day;
	alm_tm->tm_mon  = rtc_reg_list->r_yy_mm_dd.month;
	alm_tm->tm_year = rtc_reg_list->r_yy_mm_dd.year;

	if (alm_tm->tm_year < 70) {
		pr_info("%s(%d): year < 70, so add 100, to check\n", __func__, __LINE__);
		alm_tm->tm_year += 100;	/* assume we are in 1970...2100 */
	}

	/* month 1~12 in spec while 0~11 in linux system */
	alm_tm->tm_mon -= 1;

	if(rtc_reg_list->a_irq_enable.alarm_cnt_irq_en) {
		pr_info("%s(%d): rtc_wkalrm->enable condition to check\n", __func__, __LINE__);
		alrm->enabled = 1;
	}
	pr_info("%s(%d): get alarm time %d-%d-%d %d:%d:%d\n", __func__, __LINE__, alm_tm->tm_year + 1900,
		alm_tm->tm_mon + 1, alm_tm->tm_mday, alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);
	return 0;
}

static int sunxi_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	struct rtc_time tm_now;
	unsigned long time_now = 0;
	unsigned long time_set = 0;
	unsigned long time_gap = 0;
	unsigned long time_gap_day = 0;
	unsigned long time_gap_hour = 0;
	unsigned long time_gap_minute = 0;
	unsigned long time_gap_second = 0;
	int ret = 0;

	pr_info("%s(%d): time to set %d-%d-%d %d:%d:%d\n", __func__, __LINE__, tm->tm_year + 1900,
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	ret = sunxi_rtc_gettime(dev, &tm_now);

	pr_info("%s(%d): get current time: %d-%d-%d %d:%d:%d\n", __func__, __LINE__, tm_now.tm_year + 1900,
		tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

	ret = rtc_tm_to_time(tm, &time_set);
	ret = rtc_tm_to_time(&tm_now, &time_now);
	if(time_set <= time_now){
		dev_err(dev, "%s(%d) err: the day has been passed!\n", __func__, __LINE__);
		return -EINVAL;
	}
	/* calc alarm gap from now */
	time_gap = time_set - time_now;
	time_gap_day = time_gap/(3600*24); /* day */
	time_gap_hour = (time_gap - time_gap_day*24*60*60)/3600; /* hour */
	time_gap_minute = (time_gap - time_gap_day*24*60*60 - time_gap_hour*60*60)/60; /* minute */
	time_gap_second = time_gap - time_gap_day*24*60*60 - time_gap_hour*60*60-time_gap_minute*60; /* second */
	if(time_gap_day > 255) {
		dev_err(dev, "%s(%d) err: the day gap exceed 255\n", __func__, __LINE__);
		return -EINVAL;
	}
	pr_info("%s(%d): get gap day %d, hour %d, minute %d, second %d\n", __func__, __LINE__,
		(int)time_gap_day, (int)time_gap_hour, (int)time_gap_minute, (int)time_gap_second);

	/* diable alarm irq */
	sunxi_rtc_setaie(0);

	/* clear the alarm count value */
	writel(0x00000000, &rtc_reg_list->a_cnt_dd_hh_mm_ss);
	__udelay(100);

	/* rewrite the alarm count value */
	writel(0, &rtc_reg_list->a_cnt_dd_hh_mm_ss);
	rtc_reg_list->a_cnt_dd_hh_mm_ss.day = time_gap_day;
	rtc_reg_list->a_cnt_dd_hh_mm_ss.hour = time_gap_hour;
	rtc_reg_list->a_cnt_dd_hh_mm_ss.minute = time_gap_minute;
	rtc_reg_list->a_cnt_dd_hh_mm_ss.second = time_gap_second;

	/* disable alarm wk/cnt irq */
	writel(0x00000000, &rtc_reg_list->a_irq_enable);

	/* enable alarm counter irq */
	rtc_reg_list->a_irq_enable.alarm_cnt_irq_en = 1;

	if(alrm->enabled != 1)
		pr_err("%s(%d) maybe err: the rtc counter interrupt isnot enable!\n", __func__, __LINE__);

	/* decided whether we should start the counter to down count */
	sunxi_rtc_setaie(alrm->enabled);

	pr_info("%s(%d) end\n", __func__, __LINE__);
	return 0;
}

static int sunxi_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	if(!enabled)
		sunxi_rtc_setaie(enabled);
	return 0;
}

/* irq handlers, irq no. is shared with timer2 */
static irqreturn_t sunxi_rtc_alarmirq(int irq, void *id)
{
	struct rtc_device *rdev = id;
	u32 val;

	pr_info("%s, line:%d\n", __func__, __LINE__);

	/* judge the int is whether ours */
	val = rtc_reg_list->a_irq_status.cnt_irq_pend | rtc_reg_list->a_irq_status.wk_irq_pend;
	if(val) {
		/* clear alarm count pending */
		if(rtc_reg_list->a_irq_status.cnt_irq_pend)
			rtc_reg_list->a_irq_status.cnt_irq_pend = 1;
		WARN_ON(unlikely(rtc_reg_list->a_irq_status.cnt_irq_pend)); /* warn if not cleared success */

		/* liugang add, clear alarm wk pending */
		if(rtc_reg_list->a_irq_status.wk_irq_pend) {
			pr_info("%s(%d): to check, add clear alarm wk pending, liugang\n", __func__, __LINE__);
			rtc_reg_list->a_irq_status.wk_irq_pend = 1;
			WARN_ON(unlikely(rtc_reg_list->a_irq_status.wk_irq_pend));
		}

		rtc_update_irq(rdev, 1, RTC_AF | RTC_IRQF);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

int rtc_hw_init(void)
{
	pr_info("%s(%d): start\n", __func__, __LINE__);

	/*
	 * select rtc clock source
	 * on fpga board, internal 32k clk src is the default, and can not be changed
	 */
	rtc_reg_list->losc_ctl.clk32k_auto_swt_en = 0; /* disable auto switch */
	rtc_reg_list->losc_ctl.osc32k_src_sel = 1; /* external 32768hz osc */
	rtc_reg_list->losc_ctl.key_field = 0x16AA;
	rtc_reg_list->losc_ctl.ext_losc_gsm = 2; /* external 32768hz osc gsm: 0b10 */
	__udelay(100);
	/* check set result */
	if(1 != rtc_reg_list->losc_ctl.osc32k_src_sel) {
		pr_err("%s(%d) err: set clksrc to external losc failed! rtc time will be wrong\n", __func__, __LINE__);
		losc_err_flag = 1;
	}

	/* clear the alarm count value */
	writel(0x00000000, &rtc_reg_list->a_cnt_dd_hh_mm_ss);

	/* disable alarm, not generate irq pending */
	writel(0x00000000, &rtc_reg_list->a_enable);

	/* disable alarm week/cnt irq, unset to cpu */
	writel(0x00000000, &rtc_reg_list->a_irq_enable);

	/* clear alarm week/cnt irq pending */
	writel(3, &rtc_reg_list->a_irq_status);
	pr_info("%s(%d): end\n", __func__, __LINE__);
	return 0;
}
