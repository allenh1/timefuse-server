#include "event_struct.hpp"

/** 
 * Attempts to schedule an event that fits between two
 * calendar events having duration d.
 * 
 * @param e1 Earlier event.
 * @param e2 Later event.
 * @param d Duration of the desired event.
 * @param ok If this pointer is non-null, then this gets
 * the value true if an event is found in the given window.
 * 
 * @return The scheduled event.
 */
calendar_event schedule_between(const calendar_event & e1,
								const calendar_event & e2,
								const int & d, bool * ok)
{
	/* find the midpoint day */
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
		if (ok != NULL) *ok = true;
		return to_return;
	}

	/* otherwise, we're dealing with events on the dame day */
	QTime end1 = e1.time.addSecs(e1.duration * 60);
	if (end1.secsTo(e2.time) < (d * 60)) {
		/* this is not good */
		if (ok != NULL) *ok = false;
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

	/* set and return */
	if (ok != NULL) *ok = true;
	return to_return;
}
