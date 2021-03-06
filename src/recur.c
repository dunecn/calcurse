/*
 * Calcurse - text-based organizer
 *
 * Copyright (c) 2004-2011 calcurse Development Team <misc@calcurse.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer in the documentation and/or other
 *        materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Send your feedback or comments to : misc@calcurse.org
 * Calcurse home page : http://calcurse.org
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

#include "calcurse.h"

llist_ts_t                 recur_alist_p;
llist_t                    recur_elist;
static struct recur_event  bkp_cut_recur_event;
static struct recur_apoint bkp_cut_recur_apoint;

static void
free_exc (struct excp *exc)
{
  mem_free (exc);
}

static void
free_exc_list (llist_t *exc)
{
  LLIST_FREE_INNER (exc, free_exc);
  LLIST_FREE (exc);
}

static int
exc_cmp_day (struct excp *a, struct excp *b)
{
  return (a->st < b->st ? -1 : (a->st == b->st ? 0 : 1));
}

static void
recur_add_exc (llist_t *exc, long day)
{
  struct excp *o = mem_malloc (sizeof (struct excp));
  o->st = day;

  LLIST_ADD_SORTED (exc, o, exc_cmp_day);
}

static void
exc_dup (llist_t *in, llist_t *exc)
{
  llist_item_t *i;

  LLIST_INIT (in);

  if (exc)
    {
      LLIST_FOREACH (exc, i)
        {
          struct excp *p = LLIST_GET_DATA (i);
          recur_add_exc (in, p->st);
        }
    }
}

void
recur_event_free_bkp (enum eraseflg flag)
{
  if (bkp_cut_recur_event.mesg)
    {
      mem_free (bkp_cut_recur_event.mesg);
      bkp_cut_recur_event.mesg = 0;
    }
  if (bkp_cut_recur_event.rpt)
    {
      mem_free (bkp_cut_recur_event.rpt);
      bkp_cut_recur_event.rpt = 0;
    }
  free_exc_list (&bkp_cut_recur_event.exc);
  erase_note (&bkp_cut_recur_event.note, flag);
}

void
recur_apoint_free_bkp (enum eraseflg flag)
{
  if (bkp_cut_recur_apoint.mesg)
    {
      mem_free (bkp_cut_recur_apoint.mesg);
      bkp_cut_recur_apoint.mesg = 0;
    }
  if (bkp_cut_recur_apoint.rpt)
    {
      mem_free (bkp_cut_recur_apoint.rpt);
      bkp_cut_recur_apoint.rpt = 0;
    }
  free_exc_list (&bkp_cut_recur_apoint.exc);
  erase_note (&bkp_cut_recur_apoint.note, flag);
}

static void
recur_event_dup (struct recur_event *in, struct recur_event *bkp)
{
  EXIT_IF (!in || !bkp, _("null pointer"));

  bkp->id = in->id;
  bkp->day = in->day;
  bkp->mesg = mem_strdup (in->mesg);

  bkp->rpt = mem_malloc (sizeof (struct rpt));
  bkp->rpt->type = in->rpt->type;
  bkp->rpt->freq = in->rpt->freq;
  bkp->rpt->until = in->rpt->until;

  exc_dup (&bkp->exc, &in->exc);

  if (in->note)
    bkp->note = mem_strdup (in->note);
}

static void
recur_apoint_dup (struct recur_apoint *in, struct recur_apoint *bkp)
{
  EXIT_IF (!in || !bkp, _("null pointer"));

  bkp->start = in->start;
  bkp->dur = in->dur;
  bkp->state = in->state;
  bkp->mesg = mem_strdup (in->mesg);

  bkp->rpt = mem_malloc (sizeof (struct rpt));
  bkp->rpt->type = in->rpt->type;
  bkp->rpt->freq = in->rpt->freq;
  bkp->rpt->until = in->rpt->until;

  exc_dup (&bkp->exc, &in->exc);

  if (in->note)
    bkp->note = mem_strdup (in->note);
}

void
recur_apoint_llist_init (void)
{
  LLIST_TS_INIT (&recur_alist_p);
}

static void
recur_apoint_free (struct recur_apoint *rapt)
{
  mem_free (rapt->mesg);
  if (rapt->note)
    mem_free (rapt->note);
  if (rapt->rpt)
    mem_free (rapt->rpt);
  free_exc_list (&rapt->exc);
  mem_free (rapt);
}

static void
recur_event_free (struct recur_event *rev)
{
  mem_free (rev->mesg);
  if (rev->note)
    mem_free (rev->note);
  if (rev->rpt)
    mem_free (rev->rpt);
  free_exc_list (&rev->exc);
  mem_free (rev);
}

void
recur_apoint_llist_free (void)
{
  LLIST_TS_FREE_INNER (&recur_alist_p, recur_apoint_free);
  LLIST_TS_FREE (&recur_alist_p);
}

void
recur_event_llist_free (void)
{
  LLIST_FREE_INNER (&recur_elist, recur_event_free);
  LLIST_FREE (&recur_elist);
}

static int
recur_apoint_cmp_start (struct recur_apoint *a, struct recur_apoint *b)
{
  return (a->start < b->start ? -1 : (a->start == b->start ? 0 : 1));
}

static int
recur_event_cmp_day (struct recur_event *a, struct recur_event *b)
{
  return (a->day < b->day ? -1 : (a->day == b->day ? 0 : 1));
}

/* Insert a new recursive appointment in the general linked list */
struct recur_apoint *
recur_apoint_new (char *mesg, char *note, long start, long dur, char state,
                  int type, int freq, long until, llist_t *except)
{
  struct recur_apoint *rapt = mem_malloc (sizeof (struct recur_apoint));

  rapt->rpt = mem_malloc (sizeof (struct rpt));
  rapt->mesg = mem_strdup (mesg);
  rapt->note = (note != NULL) ? mem_strdup (note) : 0;
  rapt->start = start;
  rapt->state = state;
  rapt->dur = dur;
  rapt->rpt->type = type;
  rapt->rpt->freq = freq;
  rapt->rpt->until = until;
  if (except)
    {
      exc_dup (&rapt->exc, except);
      free_exc_list (except);
    }
  else
    LLIST_INIT (&rapt->exc);

  LLIST_TS_LOCK (&recur_alist_p);
  LLIST_TS_ADD_SORTED (&recur_alist_p, rapt, recur_apoint_cmp_start);
  LLIST_TS_UNLOCK (&recur_alist_p);

  return rapt;
}

/* Insert a new recursive event in the general linked list */
struct recur_event *
recur_event_new (char *mesg, char *note, long day, int id, int type, int freq,
                 long until, llist_t *except)
{
  struct recur_event *rev = mem_malloc (sizeof (struct recur_event));

  rev->rpt = mem_malloc (sizeof (struct rpt));
  rev->mesg = mem_strdup (mesg);
  rev->note = (note != NULL) ? mem_strdup (note) : 0;
  rev->day = day;
  rev->id = id;
  rev->rpt->type = type;
  rev->rpt->freq = freq;
  rev->rpt->until = until;
  if (except)
    {
      exc_dup (&rev->exc, except);
      free_exc_list (except);
    }
  else
    LLIST_INIT (&rev->exc);

  LLIST_ADD_SORTED (&recur_elist, rev, recur_event_cmp_day);

  return rev;
}

/*
 * Correspondance between the defines on recursive type,
 * and the letter to be written in file.
 */
char
recur_def2char (enum recur_type define)
{
  char recur_char;

  switch (define)
    {
    case RECUR_DAILY:
      recur_char = 'D';
      break;
    case RECUR_WEEKLY:
      recur_char = 'W';
      break;
    case RECUR_MONTHLY:
      recur_char = 'M';
      break;
    case RECUR_YEARLY:
      recur_char = 'Y';
      break;
    default:
      EXIT (_("unknown repetition type"));
      return 0;
    }

  return (recur_char);
}

/*
 * Correspondance between the letters written in file and the defines
 * concerning the recursive type.
 */
int
recur_char2def (char type)
{
  int recur_def;

  switch (type)
    {
    case 'D':
      recur_def = RECUR_DAILY;
      break;
    case 'W':
      recur_def = RECUR_WEEKLY;
      break;
    case 'M':
      recur_def = RECUR_MONTHLY;
      break;
    case 'Y':
      recur_def = RECUR_YEARLY;
      break;
    default:
      EXIT (_("unknown character"));
      return 0;
    }
  return (recur_def);
}

/* Write days for which recurrent items should not be repeated. */
static void
recur_write_exc (llist_t *lexc, FILE *f)
{
  llist_item_t *i;
  struct tm *lt;
  time_t t;
  int st_mon, st_day, st_year;

  LLIST_FOREACH (lexc, i)
    {
      struct excp *exc = LLIST_GET_DATA (i);
      t = exc->st;
      lt = localtime (&t);
      st_mon = lt->tm_mon + 1;
      st_day = lt->tm_mday;
      st_year = lt->tm_year + 1900;
      (void)fprintf (f, " !%02u/%02u/%04u", st_mon, st_day, st_year);
    }
}

/* Load the recursive appointment description */
struct recur_apoint *
recur_apoint_scan (FILE *f, struct tm start, struct tm end, char type,
                   int freq, struct tm until, char *note, llist_t *exc,
                   char state)
{
  char buf[BUFSIZ], *nl;
  time_t tstart, tend, tuntil;

  /* Read the appointment description */
  (void)fgets (buf, sizeof buf, f);
  nl = strchr (buf, '\n');
  if (nl)
    {
      *nl = '\0';
    }
  start.tm_sec = end.tm_sec = 0;
  start.tm_isdst = end.tm_isdst = -1;
  start.tm_year -= 1900;
  start.tm_mon--;
  end.tm_year -= 1900;
  end.tm_mon--;
  tstart = mktime (&start);
  tend = mktime (&end);

  if (until.tm_year != 0)
    {
      until.tm_hour = 23;
      until.tm_min = 59;
      until.tm_sec = 0;
      until.tm_isdst = -1;
      until.tm_year -= 1900;
      until.tm_mon--;
      tuntil = mktime (&until);
    }
  else
    {
      tuntil = 0;
    }
  EXIT_IF (tstart == -1 || tend == -1 || tstart > tend || tuntil == -1,
           _("date error in appointment"));

  return (recur_apoint_new (buf, note, tstart, tend - tstart, state,
                            recur_char2def (type), freq, tuntil, exc));
}

/* Load the recursive events from file */
struct recur_event *
recur_event_scan (FILE *f, struct tm start, int id, char type, int freq,
                  struct tm until, char *note, llist_t *exc)
{
  char buf[BUFSIZ], *nl;
  time_t tstart, tuntil;

  /* Read the event description */
  (void)fgets (buf, sizeof buf, f);
  nl = strchr (buf, '\n');
  if (nl)
    {
      *nl = '\0';
    }
  start.tm_hour = until.tm_hour = 12;
  start.tm_min = until.tm_min = 0;
  start.tm_sec = until.tm_sec = 0;
  start.tm_isdst = until.tm_isdst = -1;
  start.tm_year -= 1900;
  start.tm_mon--;
  if (until.tm_year != 0)
    {
      until.tm_year -= 1900;
      until.tm_mon--;
      tuntil = mktime (&until);
    }
  else
    {
      tuntil = 0;
    }
  tstart = mktime (&start);
  EXIT_IF (tstart == -1 || tuntil == -1,
           _("date error in event"));

  return recur_event_new (buf, note, tstart, id, recur_char2def (type),
                          freq, tuntil, exc);
}

/* Writting of a recursive appointment into file. */
void
recur_apoint_write (struct recur_apoint *o, FILE *f)
{
  struct tm *lt;
  time_t t;

  t = o->start;
  lt = localtime (&t);
  (void)fprintf (f, "%02u/%02u/%04u @ %02u:%02u",
                 lt->tm_mon + 1, lt->tm_mday, 1900 + lt->tm_year,
                 lt->tm_hour, lt->tm_min);

  t = o->start + o->dur;
  lt = localtime (&t);
  (void)fprintf (f, " -> %02u/%02u/%04u @ %02u:%02u",
                 lt->tm_mon + 1, lt->tm_mday, 1900 + lt->tm_year,
                 lt->tm_hour, lt->tm_min);

  t = o->rpt->until;
  if (t == 0)
    {				/* We have an endless recurrent appointment. */
      (void)fprintf (f, " {%d%c", o->rpt->freq, recur_def2char (o->rpt->type));
    }
  else
    {
      lt = localtime (&t);
      (void)fprintf (f, " {%d%c -> %02u/%02u/%04u",
                     o->rpt->freq, recur_def2char (o->rpt->type),
                     lt->tm_mon + 1, lt->tm_mday, 1900 + lt->tm_year);
    }
  recur_write_exc (&o->exc, f);
  (void)fprintf (f, "} ");
  if (o->note != NULL)
    (void)fprintf (f, ">%s ", o->note);
  if (o->state & APOINT_NOTIFY)
    (void)fprintf (f, "!");
  else
    (void)fprintf (f, "|");
  (void)fprintf (f, "%s\n", o->mesg);
}

/* Writting of a recursive event into file. */
void
recur_event_write (struct recur_event *o, FILE *f)
{
  struct tm *lt;
  time_t t;
  int st_mon, st_day, st_year;
  int end_mon, end_day, end_year;

  t = o->day;
  lt = localtime (&t);
  st_mon = lt->tm_mon + 1;
  st_day = lt->tm_mday;
  st_year = lt->tm_year + 1900;
  t = o->rpt->until;
  if (t == 0)
    {				/* We have an endless recurrent event. */
      (void)fprintf (f, "%02u/%02u/%04u [%d] {%d%c",
                     st_mon, st_day, st_year, o->id, o->rpt->freq,
                     recur_def2char (o->rpt->type));
    }
  else
    {
      lt = localtime (&t);
      end_mon = lt->tm_mon + 1;
      end_day = lt->tm_mday;
      end_year = lt->tm_year + 1900;
      (void)fprintf (f, "%02u/%02u/%04u [%d] {%d%c -> %02u/%02u/%04u",
                     st_mon, st_day, st_year, o->id,
                     o->rpt->freq, recur_def2char (o->rpt->type),
                     end_mon, end_day, end_year);
    }
  recur_write_exc (&o->exc, f);
  (void)fprintf (f, "} ");
  if (o->note != NULL)
    (void)fprintf (f, ">%s ", o->note);
  (void)fprintf (f, "%s\n", o->mesg);
}

/* Write recursive items to file. */
void
recur_save_data (FILE *f)
{
  llist_item_t *i;

  LLIST_FOREACH (&recur_elist, i)
    {
      struct recur_event *rev = LLIST_GET_DATA (i);
      recur_event_write (rev, f);
    }

  LLIST_TS_LOCK (&recur_alist_p);
  LLIST_TS_FOREACH (&recur_alist_p, i)
    {
      struct recur_apoint *rapt = LLIST_GET_DATA (i);
      recur_apoint_write (rapt, f);
    }
  LLIST_TS_UNLOCK (&recur_alist_p);
}


/*
 * The two following defines together with the diff_days, diff_weeks,
 * diff_months and diff_years functions were provided by Lukas Fleischer to
 * correct the wrong calculation of recurrent dates after a turn of year.
 */
#define BC(start, end, bs)                                              \
  (((end) - (start) + ((start) % bs) - ((end) % bs)) / bs               \
   + ((((start) % bs) == 0) ? 1 : 0))

#define LEAPCOUNT(start, end)                                           \
  (BC(start, end, 4) - BC(start, end, 100) + BC(start, end, 400))


/* Calculate the difference in days between two dates. */
static long
diff_days (struct tm lt_start, struct tm lt_end)
{
  long diff;

  if (lt_end.tm_year < lt_start.tm_year)
    return 0;

  diff = lt_end.tm_yday - lt_start.tm_yday;

  if (lt_end.tm_year > lt_start.tm_year)
    {
      diff += (lt_end.tm_year - lt_start.tm_year) * YEARINDAYS;
      diff += LEAPCOUNT (lt_start.tm_year + TM_YEAR_BASE,
                         lt_end.tm_year + TM_YEAR_BASE - 1);
    }

  return diff;
}

/* Calculate the difference in weeks between two dates. */
static long
diff_weeks (struct tm lt_start, struct tm lt_end)
{
  return diff_days (lt_start, lt_end) / WEEKINDAYS;
}

/* Calculate the difference in months between two dates. */
static long
diff_months (struct tm lt_start, struct tm lt_end)
{
  long diff;

  if (lt_end.tm_year < lt_start.tm_year)
    return 0;

  diff = lt_end.tm_mon - lt_start.tm_mon;
  diff += (lt_end.tm_year - lt_start.tm_year) * YEARINMONTHS;

  return diff;
}

/* Calculate the difference in years between two dates. */
static long
diff_years (struct tm lt_start, struct tm lt_end)
{
  return lt_end.tm_year - lt_start.tm_year;
}

static int
exc_inday (struct excp *exc, long day_start)
{
  return (exc->st >= day_start && exc->st < day_start + DAYINSEC);
}

/*
 * Check if the recurrent item belongs to the selected day,
 * and if yes, return the real start time.
 *
 * This function was improved thanks to Tony's patch.
 * Thanks also to youshe for reporting daylight saving time related problems.
 * And finally thanks to Lukas for providing a patch to correct the wrong
 * calculation of recurrent dates after a turn of years.
 */
unsigned
recur_item_inday (long item_start, llist_t *item_exc, int rpt_type,
                  int rpt_freq, long rpt_until, long day_start)
{
  struct date start_date;
  long day_end, diff;
  struct tm lt_item, lt_day;
  time_t t;

  day_end = day_start + DAYINSEC;
  t = day_start;
  lt_day = *localtime (&t);

  if (LLIST_FIND_FIRST (item_exc, day_start, exc_inday))
    return 0;

  if (rpt_until == 0)		/* we have an endless recurrent item */
    rpt_until = day_end;

  if (item_start > day_end || rpt_until < day_start)
    return (0);

  t = item_start;
  lt_item = *localtime (&t);

  switch (rpt_type)
    {
    case RECUR_DAILY:
      diff = diff_days (lt_item, lt_day);
      if (diff % rpt_freq != 0)
        return (0);
      lt_item.tm_mday = lt_day.tm_mday;
      lt_item.tm_mon = lt_day.tm_mon;
      lt_item.tm_year = lt_day.tm_year;
      break;
    case RECUR_WEEKLY:
      if (lt_item.tm_wday != lt_day.tm_wday)
        return (0);
      else
        {
          diff = diff_weeks (lt_item, lt_day);
          if (diff % rpt_freq != 0)
            return (0);
        }
      lt_item.tm_mday = lt_day.tm_mday;
      lt_item.tm_mon = lt_day.tm_mon;
      lt_item.tm_year = lt_day.tm_year;
      break;
    case RECUR_MONTHLY:
      diff = diff_months (lt_item, lt_day);
      if (diff % rpt_freq != 0)
        return (0);
      lt_item.tm_mon = lt_day.tm_mon;
      lt_item.tm_year = lt_day.tm_year;
      break;
    case RECUR_YEARLY:
      diff = diff_years (lt_item, lt_day);
      if (diff % rpt_freq != 0)
        return (0);
      lt_item.tm_year = lt_day.tm_year;
      break;
    default:
      EXIT (_("unknown item type"));
    }
  start_date.dd = lt_item.tm_mday;
  start_date.mm = lt_item.tm_mon + 1;
  start_date.yyyy = lt_item.tm_year + 1900;
  item_start = date2sec (start_date, lt_item.tm_hour, lt_item.tm_min);

  if (item_start < day_end && item_start >= day_start)
    return (item_start);
  else
    return (0);
}

unsigned
recur_apoint_inday(struct recur_apoint *rapt, long day_start)
{
  return recur_item_inday (rapt->start, &rapt->exc, rapt->rpt->type,
                           rapt->rpt->freq, rapt->rpt->until, day_start);
}

unsigned
recur_event_inday(struct recur_event *rev, long day_start)
{
  return recur_item_inday (rev->day, &rev->exc, rev->rpt->type, rev->rpt->freq,
                           rev->rpt->until, day_start);
}

/*
 * Delete a recurrent event from the list (if delete_whole is not null),
 * or delete only one occurence of the recurrent event.
 */
void
recur_event_erase (long start, unsigned num, unsigned delete_whole,
                   enum eraseflg flag)
{
  llist_item_t *i;

  i = LLIST_FIND_NTH (&recur_elist, num, start, recur_event_inday);

  if (!i)
    EXIT (_("event not found"));
  struct recur_event *rev = LLIST_GET_DATA (i);

  if (delete_whole)
    {
      switch (flag)
        {
        case ERASE_FORCE_ONLY_NOTE:
          erase_note (&rev->note, flag);
          break;
        case ERASE_CUT:
          recur_event_free_bkp (ERASE_FORCE);
          recur_event_dup (rev, &bkp_cut_recur_event);
          erase_note (&rev->note, ERASE_FORCE_KEEP_NOTE);
          /* FALLTHROUGH */
        default:
          LLIST_REMOVE (&recur_elist, i);
          mem_free (rev->mesg);
          if (rev->rpt)
            {
              mem_free (rev->rpt);
              rev->rpt = 0;
            }
          free_exc_list (&rev->exc);
          if (flag != ERASE_FORCE_KEEP_NOTE && flag != ERASE_CUT)
            erase_note (&rev->note, flag);
          mem_free (rev);
          break;
        }
    }
  else
    recur_add_exc (&rev->exc, start);
}

/*
 * Delete a recurrent appointment from the list (if delete_whole is not null),
 * or delete only one occurence of the recurrent appointment.
 */
void
recur_apoint_erase (long start, unsigned num, unsigned delete_whole,
                    enum eraseflg flag)
{
  llist_item_t *i;
  int need_check_notify = 0;

  i = LLIST_TS_FIND_NTH (&recur_alist_p, num, start, recur_apoint_inday);

  if (!i)
    EXIT (_("appointment not found"));
  struct recur_apoint *rapt = LLIST_GET_DATA (i);

  LLIST_TS_LOCK (&recur_alist_p);
  if (notify_bar () && flag != ERASE_FORCE_ONLY_NOTE)
    need_check_notify = notify_same_recur_item (rapt);
  if (delete_whole)
    {
      switch (flag)
        {
        case ERASE_FORCE_ONLY_NOTE:
          erase_note (&rapt->note, flag);
          break;
        case ERASE_CUT:
          recur_apoint_free_bkp (ERASE_FORCE);
          recur_apoint_dup (rapt, &bkp_cut_recur_apoint);
          erase_note (&rapt->note, ERASE_FORCE_KEEP_NOTE);
          /* FALLTHROUGH */
        default:
          LLIST_TS_REMOVE (&recur_alist_p, i);
          mem_free (rapt->mesg);
          if (rapt->rpt)
            {
              mem_free (rapt->rpt);
              rapt->rpt = 0;
            }
          free_exc_list (&rapt->exc);
          if (flag != ERASE_FORCE_KEEP_NOTE && flag != ERASE_CUT)
            erase_note (&rapt->note, flag);
          mem_free (rapt);
          if (need_check_notify)
            notify_check_next_app (0);
          break;
        }
    }
  else
    {
      recur_add_exc (&rapt->exc, start);
      if (need_check_notify)
        notify_check_next_app (0);
    }
  LLIST_TS_UNLOCK (&recur_alist_p);
}

/*
 * Ask user for repetition characteristics:
 * 	o repetition type: daily, weekly, monthly, yearly
 *	o repetition frequence: every X days, weeks, ...
 *	o repetition end date
 * and then delete the selected item to recreate it as a recurrent one
 */
void
recur_repeat_item (struct conf *conf)
{
  struct tm *lt;
  time_t t;
  int ch = 0;
  int date_entered = 0;
  int year = 0, month = 0, day = 0;
  struct date until_date;
  char outstr[BUFSIZ];
  char user_input[BUFSIZ] = "";
  char *mesg_type_1 =
    _("Enter the repetition type: (D)aily, (W)eekly, (M)onthly, (Y)early");
  char *mesg_type_2 = _("[D/W/M/Y] ");
  char *mesg_freq_1 = _("Enter the repetition frequence:");
  char *mesg_wrong_freq = _("The frequence you entered is not valid.");
  char *mesg_until_1 =
    _("Enter the ending date: [%s] or '0' for an endless repetition");
  char *mesg_wrong_1 = _("The entered date is not valid.");
  char *mesg_wrong_2 =
    _("Possible formats are [%s] or '0' for an endless repetition");
  char *wrong_type_1 = _("This item is already a repeated one.");
  char *wrong_type_2 = _("Press [ENTER] to continue.");
  char *mesg_older =
    _("Sorry, the date you entered is older than the item start time.");
  int type = 0, freq = 0;
  int item_nb;
  struct day_item *p;
  struct recur_apoint *ra;
  long until, date;

  item_nb = apoint_hilt ();
  p = day_get_item (item_nb);
  if (p->type != APPT && p->type != EVNT)
    {
      status_mesg (wrong_type_1, wrong_type_2);
      (void)wgetch (win[STA].p);
      return;
    }

  while ((ch != 'D') && (ch != 'W') && (ch != 'M')
         && (ch != 'Y') && (ch != ESCAPE))
    {
      status_mesg (mesg_type_1, mesg_type_2);
      ch = wgetch (win[STA].p);
      ch = toupper (ch);
    }
  if (ch == ESCAPE)
    {
      return;
    }
  else
    {
      type = recur_char2def (ch);
    }

  while (freq == 0)
    {
      status_mesg (mesg_freq_1, "");
      if (getstring (win[STA].p, user_input, BUFSIZ, 0, 1) == GETSTRING_VALID)
        {
          freq = atoi (user_input);
          if (freq == 0)
            {
              status_mesg (mesg_wrong_freq, wrong_type_2);
              (void)wgetch (win[STA].p);
            }
          user_input[0] = '\0';
        }
      else
        return;
    }

  while (!date_entered)
    {
      (void)snprintf (outstr, BUFSIZ, mesg_until_1,
                      DATEFMT_DESC (conf->input_datefmt));
      status_mesg (_(outstr), "");
      if (getstring (win[STA].p, user_input, BUFSIZ, 0, 1) == GETSTRING_VALID)
        {
          if (strlen (user_input) == 1 && strncmp (user_input, "0", 1) == 0)
            {
              until = 0;
              date_entered = 1;
            }
          else
            {
              if (parse_date (user_input, conf->input_datefmt,
                              &year, &month, &day, calendar_get_slctd_day ()))
                {
                  t = p->start;
                  lt = localtime (&t);
                  until_date.dd = day;
                  until_date.mm = month;
                  until_date.yyyy = year;
                  until = date2sec (until_date, lt->tm_hour, lt->tm_min);
                  if (until < p->start)
                    {
                      status_mesg (mesg_older, wrong_type_2);
                      (void)wgetch (win[STA].p);
                      date_entered = 0;
                    }
                  else
                    {
                      date_entered = 1;
                    }
                }
              else
                {
                  (void)snprintf (outstr, BUFSIZ, mesg_wrong_2,
                                  DATEFMT_DESC (conf->input_datefmt));
                  status_mesg (mesg_wrong_1, _(outstr));
                  (void)wgetch (win[STA].p);
                  date_entered = 0;
                }
            }
        }
      else
        return;
    }

  date = calendar_get_slctd_day_sec ();
  if (p->type == EVNT)
    {
      (void)recur_event_new (p->mesg, p->note, p->start, p->evnt_id,
                             type, freq, until, NULL);
    }
  else if (p->type == APPT)
    {
      ra = recur_apoint_new (p->mesg, p->note, p->start, p->appt_dur,
                             p->state, type, freq, until, NULL);
      if (notify_bar ())
        notify_check_repeated (ra);
    }
  else
    {
      EXIT (_("wrong item type"));
      /* NOTREACHED */
    }
  day_erase_item (date, item_nb, ERASE_FORCE_KEEP_NOTE);
}

/*
 * Read days for which recurrent items must not be repeated
 * (such days are called exceptions).
 */
void
recur_exc_scan (llist_t *lexc, FILE *data_file)
{
  int c = 0;
  struct tm day;

  LLIST_INIT (lexc);
  while ((c = getc (data_file)) == '!')
    {
      (void)ungetc (c, data_file);
      if (fscanf (data_file, "!%u / %u / %u ",
                  &day.tm_mon, &day.tm_mday, &day.tm_year) != 3)
        {
          EXIT (_("syntax error in item date"));
        }
      day.tm_hour = 12;
      day.tm_min = day.tm_sec = 0;
      day.tm_isdst = -1;
      day.tm_year -= 1900;
      day.tm_mon--;
      struct excp *exc = mem_malloc (sizeof (struct excp));
      exc->st = mktime (&day);
      LLIST_ADD (lexc, exc);
    }
}

static int
recur_apoint_starts_before (struct recur_apoint *rapt, long time)
{
  return (rapt->start < time);
}

/*
 * Look in the appointment list if we have an item which starts before the item
 * stored in the notify_app structure (which is the next item to be notified).
 */
struct notify_app *
recur_apoint_check_next (struct notify_app *app, long start, long day)
{
  llist_item_t *i;
  long real_recur_start_time;

  LLIST_TS_LOCK (&recur_alist_p);
  LLIST_TS_FIND_FOREACH (&recur_alist_p, app->time, recur_apoint_starts_before, i)
    {
      struct recur_apoint *rapt = LLIST_TS_GET_DATA (i);

      real_recur_start_time = recur_apoint_inday(rapt, day);
      if (real_recur_start_time > start)
        {
          app->time = real_recur_start_time;
          app->txt = mem_strdup (rapt->mesg);
          app->state = rapt->state;
          app->got_app = 1;
        }
    }
  LLIST_TS_UNLOCK (&recur_alist_p);

  return (app);
}

/* Returns a structure containing the selected recurrent appointment. */
struct recur_apoint *
recur_get_apoint (long date, int num)
{
  llist_item_t *i = LLIST_TS_FIND_NTH (&recur_alist_p, num, date,
                                      recur_apoint_inday);

  if (i)
    return LLIST_TS_GET_DATA (i);

  EXIT (_("item not found"));
  /* NOTREACHED */
}

/* Returns a structure containing the selected recurrent event. */
struct recur_event *
recur_get_event (long date, int num)
{
  llist_item_t *i = LLIST_FIND_NTH (&recur_elist, num, date,
                                    recur_event_inday);

  if (i)
    return LLIST_GET_DATA (i);

  EXIT (_("item not found"));
  /* NOTREACHED */
}

/* Switch recurrent item notification state. */
void
recur_apoint_switch_notify (long date, int recur_nb)
{
  llist_item_t *i;

  LLIST_TS_LOCK (&recur_alist_p);
  i = LLIST_TS_FIND_NTH (&recur_alist_p, recur_nb, date, recur_apoint_inday);

  if (!i)
    EXIT (_("item not found"));
  struct recur_apoint *rapt = LLIST_TS_GET_DATA (i);

  rapt->state ^= APOINT_NOTIFY;

  if (notify_bar ())
    notify_check_repeated (rapt);

  LLIST_TS_UNLOCK (&recur_alist_p);
}

void
recur_event_paste_item (void)
{
  long new_start, time_shift;
  llist_item_t *i;

  new_start = date2sec (*calendar_get_slctd_day (), 12, 0);
  time_shift = new_start - bkp_cut_recur_event.day;

  bkp_cut_recur_event.day += time_shift;
  if (bkp_cut_recur_event.rpt->until != 0)
    bkp_cut_recur_event.rpt->until += time_shift;
  LLIST_FOREACH (&bkp_cut_recur_event.exc, i)
    {
      struct excp *exc = LLIST_GET_DATA (i);
      exc->st += time_shift;
    }

  (void)recur_event_new (bkp_cut_recur_event.mesg, bkp_cut_recur_event.note,
                         bkp_cut_recur_event.day, bkp_cut_recur_event.id,
                         bkp_cut_recur_event.rpt->type,
                         bkp_cut_recur_event.rpt->freq,
                         bkp_cut_recur_event.rpt->until,
                         &bkp_cut_recur_event.exc);
  recur_event_free_bkp (ERASE_FORCE_KEEP_NOTE);
}

void
recur_apoint_paste_item (void)
{
  long new_start, time_shift;
  llist_item_t *i;

  new_start = date2sec (*calendar_get_slctd_day (),
                        get_item_hour (bkp_cut_recur_apoint.start),
                        get_item_min (bkp_cut_recur_apoint.start));
  time_shift = new_start - bkp_cut_recur_apoint.start;

  bkp_cut_recur_apoint.start += time_shift;
  if (bkp_cut_recur_apoint.rpt->until != 0)
    bkp_cut_recur_apoint.rpt->until += time_shift;
  LLIST_FOREACH (&bkp_cut_recur_event.exc, i)
    {
      struct excp *exc = LLIST_GET_DATA (i);
      exc->st += time_shift;
    }

  (void)recur_apoint_new (bkp_cut_recur_apoint.mesg, bkp_cut_recur_apoint.note,
                          bkp_cut_recur_apoint.start, bkp_cut_recur_apoint.dur,
                          bkp_cut_recur_apoint.state,
                          bkp_cut_recur_apoint.rpt->type,
                          bkp_cut_recur_apoint.rpt->freq,
                          bkp_cut_recur_apoint.rpt->until,
                          &bkp_cut_recur_apoint.exc);

  if (notify_bar ())
    notify_check_repeated (&bkp_cut_recur_apoint);

  recur_apoint_free_bkp (ERASE_FORCE_KEEP_NOTE);
}
