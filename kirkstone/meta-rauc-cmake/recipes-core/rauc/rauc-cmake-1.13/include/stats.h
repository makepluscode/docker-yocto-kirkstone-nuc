#pragma once

#include <glib.h>

typedef struct {
	gchar *label;
	gdouble values[64];
	guint64 count, next;
	gdouble sum;
	gdouble min, max;
} RaucStats;

RaucStats *r_stats_new(const gchar *label);

void r_stats_add(RaucStats *stats, gdouble value);

gdouble r_stats_get_avg(const RaucStats *stats);

gdouble r_stats_get_recent_avg(const RaucStats *stats);

void r_stats_show(const RaucStats *stats, const gchar *prefix);

void r_stats_free(RaucStats *stats);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucStats, r_stats_free);

/* additional functions for testing */
void r_test_stats_start(void);
void r_test_stats_stop(void);
RaucStats *r_test_stats_next(void);
