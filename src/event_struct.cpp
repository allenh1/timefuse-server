#include "event_struct.hpp"

/**
 * Schedules an event between the given times.
 *
 * @param events List of all a user's events.
 * @param index_1 Index of the first time.
 * @param index_2 Index of the second time.
 * @param d Duration of the event (in minutes).
 * @param ok Store result here, if not null.
 *
 * @return An event between the provided times that works,
 * or a blank event.
 */
calendar_event schedule_between(
  const QList<calendar_event> & events,
  const size_t & index_1,
  const size_t & index_2,
  const int & d, bool * ok)
{
  /* find the midpoint day */
  calendar_event e1 = events[index_1];
  calendar_event e2 = events[index_2];
  qint64 days_between = e1.date.daysTo(e2.date);

  if (days_between > 1) {
    /* return the middle of the day on the midpoint day */
    calendar_event to_return;
    /**
     * @todo garauntee this is not overlapping with the later date
     */
    to_return.date = e1.date.addDays(days_between >> 1);
    /* set to 4:00 PM */
    to_return.time = QTime(16, 0);
    to_return.duration = d;
    if (ok != NULL) {*ok = true;}
    return to_return;
  }

  /* otherwise, we're dealing with events on the dame day */
  QTime end1 = e1.time.addSecs(e1.duration * 60);
  if (end1.secsTo(e2.time) < (d * 60)) {
    /* this is not good */
    if (ok != NULL) {*ok = false;}
    calendar_event rip;
    return rip;
  }

  /* otherwise, we have some hope. */
  QTime middle = end1.addSecs(end1.secsTo(e2.time) >> 1);
  calendar_event to_return;
  /* copy over the date */
  to_return.date = e1.date;
  /* ((d * 60) / 2) * -1 */
  to_return.time = middle.addSecs(d * (-30));
  to_return.duration = d;

  /* iterate through the events, checking for this event */
  for (int x = 0; x < events.size(); ++x) {
    if (to_return == events[x]) {
      if (ok != NULL) {*ok = false;}
      calendar_event rip;
      return rip;
    }
  }

  /* set and return */
  if (ok != NULL) {*ok = true;}
  return to_return;
}

/**
 * Check if two events overlap.
 *
 * @param e1 First event.
 * @param e2 Second event.
 *
 * @return True if e1 conflicts with e2.
 */
bool check_overlap(
  calendar_event e1,
  calendar_event e2)
{
  /* check they are on the same day */
  if (e1.date != e2.date) {return false;}

  QTime e1_finish = e1.get_finish_time(),
    e2_finish = e2.get_finish_time();

  /* check if e1 starts during e2 */
  if (e1.time > e2.time && e1.time <= e2_finish) {return true;}

  /* check if e2 starts during e1 */
  if (e2.time > e1.time && e2.time <= e1_finish) {return true;}

  return false;
}
