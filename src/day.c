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
#include <sys/types.h>
#include <ctype.h>
#include <time.h>

#include "calcurse.h"

struct day_saved_item {
  char  start[BUFSIZ];
  char  end[BUFSIZ];
  char  state;
  char  type;
  char *mesg;
};

static llist_t                 day_items;
static struct day_saved_item   day_saved_item;

static void
day_free (struct day_item *day)
{
  mem_free (day);
}

static void
day_init_list (void)
{
  LLIST_INIT (&day_items);
}

/*
 * Free the current day linked list containing the events and appointments.
 * Must not free associated message and note, because their are not dynamically
 * allocated (only pointers to real objects are stored in this structure).
 */
void
day_free_list (void)
{
  LLIST_FREE_INNER (&day_items, day_free);
  LLIST_FREE (&day_items);
}

/* Add an event in the current day list */
static struct day_item *
day_add_event (int type, char *mesg, char *note, long nday, int id)
{
  struct day_item *day;

  day = mem_malloc (sizeof (struct day_item));
  day->mesg = mesg;
  day->note = note;
  day->type = type;
  day->appt_dur = 0;
  day->appt_pos = 0;
  day->start = nday;
  day->evnt_id = id;

  LLIST_ADD (&day_items, day);

  return day;
}

static int
day_cmp_start (struct day_item *a, struct day_item *b)
{
  if (a->type <= EVNT)
    {
      if (b->type <= EVNT)
        return 0;
      else
        return -1;
    }
  else if (b->type <= EVNT)
    return 1;
  else
    return (a->start < b->start ? -1 : (a->start == b->start ? 0 : 1));
}

/* Add an appointment in the current day list. */
static struct day_item *
day_add_apoint (int type, char *mesg, char *note, long start, long dur,
                char state, int real_pos)
{
  struct day_item *day;

  day = mem_malloc (sizeof (struct day_item));
  day->mesg = mesg;
  day->note = note;
  day->start = start;
  day->appt_dur = dur;
  day->appt_pos = real_pos;
  day->state = state;
  day->type = type;
  day->evnt_id = 0;

  LLIST_ADD_SORTED (&day_items, day, day_cmp_start);

  return day;
}

/*
 * Store the events for the selected day in structure pointed
 * by day_items. This is done by copying the events
 * from the general structure pointed by eventlist to the structure
 * dedicated to the selected day.
 * Returns the number of events for the selected day.
 */
static int
day_store_events (long date)
{
  llist_item_t *i;
  int e_nb = 0;

  LLIST_FIND_FOREACH (&eventlist, date, event_inday, i)
    {
      struct event *ev = LLIST_TS_GET_DATA (i);
      (void)day_add_event (EVNT, ev->mesg, ev->note, ev->day, ev->id);
      e_nb++;
    }

  return e_nb;
}

/*
 * Store the recurrent events for the selected day in structure pointed
 * by day_items. This is done by copying the recurrent events
 * from the general structure pointed by recur_elist to the structure
 * dedicated to the selected day.
 * Returns the number of recurrent events for the selected day.
 */
static int
day_store_recur_events (long date)
{
  llist_item_t *i;
  int e_nb = 0;

  LLIST_FIND_FOREACH (&recur_elist, date, recur_event_inday, i)
    {
      struct recur_event *rev = LLIST_TS_GET_DATA (i);
      (void)day_add_event (RECUR_EVNT, rev->mesg, rev->note, rev->day,
                           rev->id);
      e_nb++;
    }

  return e_nb;
}

/*
 * Store the apoints for the selected day in structure pointed
 * by day_items. This is done by copying the appointments
 * from the general structure pointed by alist_p to the
 * structure dedicated to the selected day.
 * Returns the number of appointments for the selected day.
 */
static int
day_store_apoints (long date)
{
  llist_item_t *i;
  int a_nb = 0;

  LLIST_TS_LOCK (&alist_p);
  LLIST_TS_FIND_FOREACH (&alist_p, date, apoint_inday, i)
    {
      struct apoint *apt = LLIST_TS_GET_DATA (i);
      (void)day_add_apoint (APPT, apt->mesg, apt->note, apt->start, apt->dur,
                            apt->state, 0);
      a_nb++;
    }
  LLIST_TS_UNLOCK (&alist_p);

  return a_nb;
}

/*
 * Store the recurrent apoints for the selected day in structure pointed
 * by day_items. This is done by copying the appointments
 * from the general structure pointed by recur_alist_p to the
 * structure dedicated to the selected day.
 * Returns the number of recurrent appointments for the selected day.
 */
static int
day_store_recur_apoints (long date)
{
  llist_item_t *i;
  int a_nb = 0;

  LLIST_TS_LOCK (&recur_alist_p);
  LLIST_TS_FIND_FOREACH (&recur_alist_p, date, recur_apoint_inday, i)
    {
      struct recur_apoint *rapt = LLIST_TS_GET_DATA (i);
      int real_start = recur_apoint_inday (rapt, date);
      (void)day_add_apoint (RECUR_APPT, rapt->mesg, rapt->note, real_start,
                            rapt->dur, rapt->state, a_nb);
      a_nb++;
    }
  LLIST_TS_UNLOCK (&recur_alist_p);

  return a_nb;
}

/*
 * Store all of the items to be displayed for the selected day.
 * Items are of four types: recursive events, normal events,
 * recursive appointments and normal appointments.
 * The items are stored in the linked list pointed by day_items
 * and the length of the new pad to write is returned.
 * The number of events and appointments in the current day are also updated.
 */
static int
day_store_items (long date, unsigned *pnb_events, unsigned *pnb_apoints)
{
  int pad_length;
  int nb_events, nb_recur_events;
  int nb_apoints, nb_recur_apoints;

  day_free_list ();
  day_init_list ();
  nb_recur_events = day_store_recur_events (date);
  nb_events = day_store_events (date);
  *pnb_events = nb_events;
  nb_recur_apoints = day_store_recur_apoints (date);
  nb_apoints = day_store_apoints (date);
  *pnb_apoints = nb_apoints;
  pad_length = (nb_recur_events + nb_events + 1 +
                3 * (nb_recur_apoints + nb_apoints));
  *pnb_apoints += nb_recur_apoints;
  *pnb_events += nb_recur_events;

  return pad_length;
}

/*
 * Store the events and appointments for the selected day, and write
 * those items in a pad. If selected day is null, then store items for current
 * day. This is useful to speed up the appointment panel update.
 */
struct day_items_nb *
day_process_storage (struct date *slctd_date, unsigned day_changed,
                     struct day_items_nb *inday)
{
  long date;
  struct date day;

  if (slctd_date)
    day = *slctd_date;
  else
    calendar_store_current_date (&day);

  date = date2sec (day, 0, 0);

  /* Inits */
  if (apad.length != 0)
    delwin (apad.ptrwin);

  /* Store the events and appointments (recursive and normal items). */
  apad.length = day_store_items (date, &inday->nb_events, &inday->nb_apoints);

  /* Create the new pad with its new length. */
  if (day_changed)
    apad.first_onscreen = 0;
  apad.ptrwin = newpad (apad.length, apad.width);

  return inday;
}

/*
 * Returns a structure of type apoint_llist_node_t given a structure of type
 * day_item_s
 */
static void
day_item_s2apoint_s (struct apoint *a, struct day_item *p)
{
  a->state = p->state;
  a->start = p->start;
  a->dur = p->appt_dur;
  a->mesg = p->mesg;
}

/*
 * Print an item date in the appointment panel.
 */
static void
display_item_date (int incolor, struct apoint *i, int type, long date,
                   int y, int x)
{
  WINDOW *win;
  char a_st[100], a_end[100];
  int recur = 0;

  win = apad.ptrwin;
  apoint_sec2str (i, type, date, a_st, a_end);
  if (type == RECUR_EVNT || type == RECUR_APPT)
    recur = 1;
  if (incolor == 0)
    custom_apply_attr (win, ATTR_HIGHEST);
  if (recur)
    if (i->state & APOINT_NOTIFY)
      mvwprintw (win, y, x, " *!%s -> %s", a_st, a_end);
    else
      mvwprintw (win, y, x, " * %s -> %s", a_st, a_end);
  else if (i->state & APOINT_NOTIFY)
    mvwprintw (win, y, x, " -!%s -> %s", a_st, a_end);
  else
    mvwprintw (win, y, x, " - %s -> %s", a_st, a_end);
  if (incolor == 0)
    custom_remove_attr (win, ATTR_HIGHEST);
}

/*
 * Print an item description in the corresponding panel window.
 */
static void
display_item (int incolor, char *msg, int recur, int note, int width, int y,
              int x)
{
  WINDOW *win;
  int ch_recur, ch_note;
  char buf[width * UTF8_MAXLEN];
  int i;

  if (width <= 0)
    return;

  win = apad.ptrwin;
  ch_recur = (recur) ? '*' : ' ';
  ch_note = (note) ? '>' : ' ';
  if (incolor == 0)
    custom_apply_attr (win, ATTR_HIGHEST);
  if (utf8_strwidth (msg) < width)
    mvwprintw (win, y, x, " %c%c%s", ch_recur, ch_note, msg);
  else
    {
      for (i = 0; msg[i] && width > 0; i++)
        {
          if (!UTF8_ISCONT (msg[i]))
            width -= utf8_width (&msg[i]);
          buf[i] = msg[i];
        }
      if (i)
        buf[i - 1] = 0;
      else
        buf[0] = 0;
      mvwprintw (win, y, x, " %c%c%s...", ch_recur, ch_note, buf);
    }
  if (incolor == 0)
    custom_remove_attr (win, ATTR_HIGHEST);
}

/*
 * Write the appointments and events for the selected day in a pad.
 * An horizontal line is drawn between events and appointments, and the
 * item selected by user is highlighted. This item is also saved inside
 * structure (pointed by day_saved_item), to be later displayed in a
 * popup window if requested.
 */
void
day_write_pad (long date, int width, int length, int incolor)
{
  llist_item_t *i;
  struct apoint a;
  int line, item_number, recur;
  const int x_pos = 0;
  unsigned draw_line = 0;

  line = item_number = 0;

  LLIST_FOREACH (&day_items, i)
    {
      struct day_item *day = LLIST_TS_GET_DATA (i);
      if (day->type == RECUR_EVNT || day->type == RECUR_APPT)
        recur = 1;
      else
        recur = 0;
      /* First print the events for current day. */
      if (day->type < RECUR_APPT)
        {
          item_number++;
          if (item_number - incolor == 0)
            {
              day_saved_item.type = day->type;
              day_saved_item.mesg = day->mesg;
            }
          display_item (item_number - incolor, day->mesg, recur,
                        (day->note != NULL) ? 1 : 0, width - 7, line, x_pos);
          line++;
          draw_line = 1;
        }
      else
        {
          /* Draw a line between events and appointments. */
          if (line > 0 && draw_line)
            {
              wmove (apad.ptrwin, line, 0);
              whline (apad.ptrwin, 0, width);
              draw_line = 0;
            }
          /* Last print the appointments for current day. */
          item_number++;
          day_item_s2apoint_s (&a, day);
          if (item_number - incolor == 0)
            {
              day_saved_item.type = day->type;
              day_saved_item.mesg = day->mesg;
              apoint_sec2str (&a, day->type, date,
                              day_saved_item.start, day_saved_item.end);
            }
          display_item_date (item_number - incolor, &a, day->type,
                             date, line + 1, x_pos);
          display_item (item_number - incolor, day->mesg, 0,
                        (day->note != NULL) ? 1 : 0, width - 7, line + 2,
                        x_pos);
          line += 3;
        }
    }
}

/* Display an item inside a popup window. */
void
day_popup_item (void)
{
  if (day_saved_item.type == EVNT || day_saved_item.type == RECUR_EVNT)
    item_in_popup (NULL, NULL, day_saved_item.mesg, _("Event :"));
  else if (day_saved_item.type == APPT || day_saved_item.type == RECUR_APPT)
    item_in_popup (day_saved_item.start, day_saved_item.end,
                   day_saved_item.mesg, _("Appointment :"));
  else
    EXIT (_("unknown item type"));
  /* NOTREACHED */
}

/*
 * Need to know if there is an item for the current selected day inside
 * calendar. This is used to put the correct colors inside calendar panel.
 */
int
day_check_if_item (struct date day)
{
  const long date = date2sec (day, 0, 0);

  if (LLIST_FIND_FIRST (&recur_elist, date, recur_event_inday))
    return (1);

  LLIST_TS_LOCK (&recur_alist_p);
  if (LLIST_TS_FIND_FIRST (&recur_alist_p, date, recur_apoint_inday))
    {
      LLIST_TS_UNLOCK (&recur_alist_p);
      return (1);
    }
  LLIST_TS_UNLOCK (&recur_alist_p);

  if (LLIST_FIND_FIRST (&eventlist, date, event_inday))
    return (1);

  LLIST_TS_LOCK (&alist_p);
  if (LLIST_TS_FIND_FIRST (&alist_p, date, apoint_inday))
    {
      LLIST_TS_UNLOCK (&alist_p);
      return (1);
    }
  LLIST_TS_UNLOCK (&alist_p);

  return (0);
}

static unsigned
fill_slices (int *slices, int slicesno, int first, int last)
{
  int i;

  if (first < 0 || last < first)
    return 0;

  if (last >= slicesno)
    last = slicesno - 1; /* Appointment spanning more than one day. */

  for (i = first; i <= last; i++)
    slices[i] = 1;

  return 1;
}

/*
 * Fill in the 'slices' vector given as an argument with 1 if there is an
 * appointment in the corresponding time slice, 0 otherwise.
 * A 24 hours day is divided into 'slicesno' number of time slices.
 */
unsigned
day_chk_busy_slices (struct date day, int slicesno, int *slices)
{
  llist_item_t *i;
  int slicelen;
  const long date = date2sec (day, 0, 0);

  slicelen = DAYINSEC / slicesno;

#define  SLICENUM(tsec)  ((tsec) / slicelen % slicesno)

  LLIST_TS_LOCK (&recur_alist_p);
  LLIST_TS_FIND_FOREACH (&recur_alist_p, date, recur_apoint_inday, i)
    {
      struct apoint *rapt = LLIST_TS_GET_DATA (i);
      long start = get_item_time (rapt->start);
      long end = get_item_time (rapt->start + rapt->dur);

      if (!fill_slices (slices, slicesno, SLICENUM (start), SLICENUM (end)))
        {
          LLIST_TS_UNLOCK (&recur_alist_p);
          return 0;
        }
    }
  LLIST_TS_UNLOCK (&recur_alist_p);

  LLIST_TS_LOCK (&alist_p);
  LLIST_TS_FIND_FOREACH (&alist_p, date, apoint_inday, i)
    {
      struct apoint *apt = LLIST_TS_GET_DATA (i);
      long start = get_item_time (apt->start);
      long end = get_item_time (apt->start + apt->dur);

      if (!fill_slices (slices, slicesno, SLICENUM (start), SLICENUM (end)))
        {
          LLIST_TS_UNLOCK (&alist_p);
          return 0;
        }
    }
  LLIST_TS_UNLOCK (&alist_p);

#undef SLICENUM
  return 1;
}

/* Request the user to enter a new time. */
static char *
day_edit_time (long time)
{
  char *timestr;
  char *msg_time = _("Enter the new time ([hh:mm] or [h:mm]) : ");
  char *enter_str = _("Press [Enter] to continue");
  char *fmt_msg = _("You entered an invalid time, should be [h:mm] or [hh:mm]");

  while (1)
    {
      status_mesg (msg_time, "");
      timestr = date_sec2date_str (time, "%H:%M");
      updatestring (win[STA].p, &timestr, 0, 1);
      if (check_time (timestr) != 1 || strlen (timestr) == 0)
        {
          status_mesg (fmt_msg, enter_str);
          (void)wgetch (win[STA].p);
        }
      else
        return (timestr);
    }
}

static void
update_start_time (long *start, long *dur)
{
  long newtime;
  unsigned hr, mn;
  int valid_date;
  char *timestr;
  char *msg_wrong_time = _("Invalid time: start time must be before end time!");
  char *msg_enter = _("Press [Enter] to continue");

  do
    {
      timestr = day_edit_time (*start);
      (void)sscanf (timestr, "%u:%u", &hr, &mn);
      mem_free (timestr);
      newtime = update_time_in_date (*start, hr, mn);
      if (newtime < *start + *dur)
        {
          *dur -= (newtime - *start);
          *start = newtime;
          valid_date = 1;
        }
      else
        {
          status_mesg (msg_wrong_time, msg_enter);
          (void)wgetch (win[STA].p);
          valid_date = 0;
        }
    }
  while (valid_date == 0);
}

static void
update_duration (long *start, long *dur)
{
  long newtime;
  unsigned hr, mn;
  char *timestr;

  timestr = day_edit_time (*start + *dur);
  (void)sscanf (timestr, "%u:%u", &hr, &mn);
  mem_free (timestr);
  newtime = update_time_in_date (*start, hr, mn);
  *dur = (newtime > *start) ? newtime - *start : DAYINSEC + newtime - *start;
}

static void
update_desc (char **desc)
{
  status_mesg (_("Enter the new item description:"), "");
  updatestring (win[STA].p, desc, 0, 1);
}

static void
update_rept (struct rpt **rpt, const long start, struct conf *conf)
{
  const int SINGLECHAR = 2;
  int ch, cancel, newfreq, date_entered;
  long newuntil;
  char outstr[BUFSIZ];
  char *typstr, *freqstr, *timstr;
  char *msg_rpt_type = _("Enter the new repetition type: (D)aily, (W)eekly, "
                         "(M)onthly, (Y)early");
  char *msg_rpt_ans = _("[D/W/M/Y] ");
  char *msg_wrong_freq = _("The frequence you entered is not valid.");
  char *msg_wrong_time = _("Invalid time: start time must be before end time!");
  char *msg_wrong_date = _("The entered date is not valid.");
  char *msg_fmts =
    "Possible formats are [%s] or '0' for an endless repetetition";
  char *msg_enter = _("Press [Enter] to continue");

  do
    {
      status_mesg (msg_rpt_type, msg_rpt_ans);
      typstr = mem_calloc (SINGLECHAR, sizeof (char));
      (void)snprintf (typstr, SINGLECHAR, "%c", recur_def2char ((*rpt)->type));
      cancel = updatestring (win[STA].p, &typstr, 0, 1);
      if (cancel)
        {
          mem_free (typstr);
          return;
        }
      else
        {
          ch = toupper (*typstr);
          mem_free (typstr);
        }
    }
  while ((ch != 'D') && (ch != 'W') && (ch != 'M') && (ch != 'Y'));

  do
    {
      status_mesg (_("Enter the new repetition frequence:"), "");
      freqstr = mem_malloc (BUFSIZ);
      (void)snprintf (freqstr, BUFSIZ, "%d", (*rpt)->freq);
      cancel = updatestring (win[STA].p, &freqstr, 0, 1);
      if (cancel)
        {
          mem_free (freqstr);
          return;
        }
      else
        {
          newfreq = atoi (freqstr);
          mem_free (freqstr);
          if (newfreq == 0)
            {
              status_mesg (msg_wrong_freq, msg_enter);
              (void)wgetch (win[STA].p);
            }
        }
    }
  while (newfreq == 0);

  do
    {
      (void)snprintf (outstr, BUFSIZ, "Enter the new ending date: [%s] or '0'",
                      DATEFMT_DESC (conf->input_datefmt));
      status_mesg (_(outstr), "");
      timstr =
          date_sec2date_str ((*rpt)->until, DATEFMT (conf->input_datefmt));
      cancel = updatestring (win[STA].p, &timstr, 0, 1);
      if (cancel)
        {
          mem_free (timstr);
          return;
        }
      if (strcmp (timstr, "0") == 0)
        {
          newuntil = 0;
          date_entered = 1;
        }
      else
        {
          struct tm *lt;
          time_t t;
          struct date new_date;
          int newmonth, newday, newyear;

          if (parse_date (timstr, conf->input_datefmt,
                          &newyear, &newmonth, &newday, calendar_get_slctd_day ()))
            {
              t = start;
              lt = localtime (&t);
              new_date.dd = newday;
              new_date.mm = newmonth;
              new_date.yyyy = newyear;
              newuntil = date2sec (new_date, lt->tm_hour, lt->tm_min);
              if (newuntil < start)
                {
                  status_mesg (msg_wrong_time, msg_enter);
                  (void)wgetch (win[STA].p);
                  date_entered = 0;
                }
              else
                date_entered = 1;
            }
          else
            {
              (void)snprintf (outstr, BUFSIZ, msg_fmts,
                              DATEFMT_DESC (conf->input_datefmt));
              status_mesg (msg_wrong_date, _(outstr));
              (void)wgetch (win[STA].p);
              date_entered = 0;
            }
        }
    }
  while (date_entered == 0);

  mem_free (timstr);
  (*rpt)->type = recur_char2def (ch);
  (*rpt)->freq = newfreq;
  (*rpt)->until = newuntil;
}

/* Edit an already existing item. */
void
day_edit_item (struct conf *conf)
{
#define STRT		'1'
#define END		'2'
#define DESC		'3'
#define REPT		'4'

  struct day_item *p;
  struct recur_event *re;
  struct event *e;
  struct recur_apoint *ra;
  struct apoint *a;
  long date;
  int item_num, ch;
  int need_check_notify = 0;

  item_num = apoint_hilt ();
  p = day_get_item (item_num);
  date = calendar_get_slctd_day_sec ();

  ch = -1;
  switch (p->type)
    {
    case RECUR_EVNT:
      re = recur_get_event (date, day_item_nb (date, item_num, RECUR_EVNT));
      status_mesg (_("Edit: (1)Description or (2)Repetition?"), "[1/2] ");
      while (ch != '1' && ch != '2' && ch != KEY_GENERIC_CANCEL)
        ch = wgetch (win[STA].p);
      switch (ch)
        {
        case '1':
          update_desc (&re->mesg);
          break;
        case '2':
          update_rept (&re->rpt, re->day, conf);
          break;
        default:
          return;
        }
      break;
    case EVNT:
      e = event_get (date, day_item_nb (date, item_num, EVNT));
      update_desc (&e->mesg);
      break;
    case RECUR_APPT:
      ra = recur_get_apoint (date, day_item_nb (date, item_num, RECUR_APPT));
      status_mesg (_("Edit: (1)Start time, (2)End time, "
                     "(3)Description or (4)Repetition?"), "[1/2/3/4] ");
      while (ch != STRT && ch != END && ch != DESC &&
             ch != REPT && ch != KEY_GENERIC_CANCEL)
        ch = wgetch (win[STA].p);
      switch (ch)
        {
        case STRT:
          need_check_notify = 1;
          update_start_time (&ra->start, &ra->dur);
          break;
        case END:
          update_duration (&ra->start, &ra->dur);
          break;
        case DESC:
          if (notify_bar ())
            need_check_notify = notify_same_recur_item (ra);
          update_desc (&ra->mesg);
          break;
        case REPT:
          need_check_notify = 1;
          update_rept (&ra->rpt, ra->start, conf);
          break;
        case KEY_GENERIC_CANCEL:
          return;
        }
      break;
    case APPT:
      a = apoint_get (date, day_item_nb (date, item_num, APPT));
      status_mesg (_("Edit: (1)Start time, (2)End time "
                     "or (3)Description?"), "[1/2/3] ");
      while (ch != STRT && ch != END && ch != DESC && ch != KEY_GENERIC_CANCEL)
        ch = wgetch (win[STA].p);
      switch (ch)
        {
        case STRT:
          need_check_notify = 1;
          update_start_time (&a->start, &a->dur);
          break;
        case END:
          update_duration (&a->start, &a->dur);
          break;
        case DESC:
          if (notify_bar ())
            need_check_notify = notify_same_item (a->start);
          update_desc (&a->mesg);
          break;
        case KEY_GENERIC_CANCEL:
          return;
        }
      break;
    }

  if (need_check_notify)
    notify_check_next_app (1);
}

/*
 * In order to erase an item, we need to count first the number of
 * items for each type (in order: recurrent events, events,
 * recurrent appointments and appointments) and then to test the
 * type of the item to be deleted.
 */
int
day_erase_item (long date, int item_number, enum eraseflg flag)
{
  struct day_item *p;
  char *erase_warning =
      _("This item is recurrent. "
        "Delete (a)ll occurences or just this (o)ne ?");
  char *note_warning =
      _("This item has a note attached to it. "
        "Delete (i)tem or just its (n)ote ?");
  char *note_choice = _("[i/n] ");
  char *erase_choice = _("[a/o] ");
  int ch, ans;
  unsigned delete_whole;

  ch = -1;
  p = day_get_item (item_number);
  if (flag == ERASE_DONT_FORCE)
    {
      ans = 0;
      if (p->note == NULL)
        ans = 'i';
      while (ans != 'i' && ans != 'n')
        {
          status_mesg (note_warning, note_choice);
          ans = wgetch (win[STA].p);
        }
      if (ans == 'i')
        flag = ERASE_FORCE;
      else
        flag = ERASE_FORCE_ONLY_NOTE;
    }
  if (p->type == EVNT)
    {
      event_delete_bynum (date, day_item_nb (date, item_number, EVNT), flag);
    }
  else if (p->type == APPT)
    {
      apoint_delete_bynum (date, day_item_nb (date, item_number, APPT), flag);
    }
  else
    {
      if (flag == ERASE_FORCE_ONLY_NOTE)
        ch = 'a';
      while ((ch != 'a') && (ch != 'o') && (ch != KEY_GENERIC_CANCEL))
        {
          status_mesg (erase_warning, erase_choice);
          ch = wgetch (win[STA].p);
        }
      if (ch == 'a')
        {
          delete_whole = 1;
        }
      else if (ch == 'o')
        {
          delete_whole = 0;
        }
      else
        {
          return (0);
        }
      if (p->type == RECUR_EVNT)
        {
          recur_event_erase (date, day_item_nb (date, item_number, RECUR_EVNT),
                             delete_whole, flag);
        }
      else
        {
          recur_apoint_erase (date, p->appt_pos, delete_whole, flag);
        }
    }
  if (flag == ERASE_FORCE_ONLY_NOTE)
    return 0;
  else
    return (p->type);
}

/* Cut an item so it can be pasted somewhere else later. */
int
day_cut_item (long date, int item_number)
{
  const int DELETE_WHOLE = 1;
  struct day_item *p;

  p = day_get_item (item_number);
  switch (p->type)
    {
    case EVNT:
      event_delete_bynum (date, day_item_nb (date, item_number, EVNT),
                          ERASE_CUT);
      break;
    case RECUR_EVNT:
      recur_event_erase (date, day_item_nb (date, item_number, RECUR_EVNT),
                         DELETE_WHOLE, ERASE_CUT);
      break;
    case APPT:
      apoint_delete_bynum (date, day_item_nb (date, item_number, APPT),
                           ERASE_CUT);
      break;
    case RECUR_APPT:
      recur_apoint_erase (date, p->appt_pos, DELETE_WHOLE, ERASE_CUT);
      break;
    default:
      EXIT (_("unknwon type"));
      /* NOTREACHED */
    }

  return p->type;
}

/* Paste a previously cut item. */
int
day_paste_item (long date, int cut_item_type)
{
  int pasted_item_type;

  pasted_item_type = cut_item_type;
  switch (cut_item_type)
    {
    case 0:
      return 0;
    case EVNT:
      event_paste_item ();
      break;
    case RECUR_EVNT:
      recur_event_paste_item ();
      break;
    case APPT:
      apoint_paste_item ();
      break;
    case RECUR_APPT:
      recur_apoint_paste_item ();
      break;
    default:
      EXIT (_("unknwon type"));
      /* NOTREACHED */
    }

  return pasted_item_type;
}

/* Returns a structure containing the selected item. */
struct day_item *
day_get_item (int item_number)
{
  return LLIST_GET_DATA (LLIST_NTH (&day_items, item_number - 1));
}

/* Returns the real item number, given its type. */
int
day_item_nb (long date, int day_num, int type)
{
  int i, nb_item[MAX_TYPES];
  llist_item_t *j;

  for (i = 0; i < MAX_TYPES; i++)
    nb_item[i] = 0;

  j = LLIST_FIRST (&day_items);
  for (i = 1; i < day_num; i++)
    {
      struct day_item *day = LLIST_TS_GET_DATA (j);
      nb_item[day->type - 1]++;
      j = LLIST_TS_NEXT (j);
    }

  return (nb_item[type - 1]);
}

/* Attach a note to an appointment or event. */
void
day_edit_note (char *editor)
{
  struct day_item *p;
  struct recur_apoint *ra;
  struct apoint *a;
  struct recur_event *re;
  struct event *e;
  long date;
  int item_num;

  item_num = apoint_hilt ();
  p = day_get_item (item_num);
  edit_note (&p->note, editor);

  date = calendar_get_slctd_day_sec ();
  switch (p->type)
    {
    case RECUR_EVNT:
      re = recur_get_event (date, day_item_nb (date, item_num, RECUR_EVNT));
      re->note = p->note;
      break;
    case EVNT:
      e = event_get (date, day_item_nb (date, item_num, EVNT));
      e->note = p->note;
      break;
    case RECUR_APPT:
      ra = recur_get_apoint (date, day_item_nb (date, item_num, RECUR_APPT));
      ra->note = p->note;
      break;
    case APPT:
      a = apoint_get (date, day_item_nb (date, item_num, APPT));
      a->note = p->note;
      break;
    }
}

/* View a note previously attached to an appointment or event */
void
day_view_note (char *pager)
{
  struct day_item *p = day_get_item (apoint_hilt ());
  view_note (p->note, pager);
}

/* Pipe an appointment or event to an external program. */
void
day_pipe_item (struct conf *conf)
{
  char cmd[BUFSIZ] = "";
  int pout;
  int pid;
  FILE *fpout;
  int item_num;
  long date;
  struct day_item *p;
  struct recur_apoint *ra;
  struct apoint *a;
  struct recur_event *re;
  struct event *e;

  status_mesg (_("Pipe item to external command:"), "");
  if (getstring (win[STA].p, cmd, BUFSIZ, 0, 1) != GETSTRING_VALID)
    return;

  wins_prepare_external ();
  if ((pid = shell_exec (NULL, &pout, cmd)))
    {
      fpout = fdopen (pout, "w");

      item_num = apoint_hilt ();
      p = day_get_item (item_num);
      date = calendar_get_slctd_day_sec ();
      switch (p->type)
        {
        case RECUR_EVNT:
          re = recur_get_event (date, day_item_nb (date, item_num, RECUR_EVNT));
          recur_event_write (re, fpout);
          break;
        case EVNT:
          e = event_get (date, day_item_nb (date, item_num, EVNT));
          event_write (e, fpout);
          break;
        case RECUR_APPT:
          ra = recur_get_apoint (date, day_item_nb (date, item_num, RECUR_APPT));
          recur_apoint_write (ra, fpout);
          break;
        case APPT:
          a = apoint_get (date, day_item_nb (date, item_num, APPT));
          apoint_write (a, fpout);
          break;
        }

      fclose (fpout);
      child_wait (NULL, &pout, pid);
      press_any_key ();
    }
  wins_unprepare_external ();
}
