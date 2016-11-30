#ifndef __EVENT_STRUCT_HPP__
#define __EVENT_STRUCT_HPP__
#include <QDate>
#include <QTime>

struct calendar_event
{
	QDate date;
	QTime time;
    int duration; /* in minutes */

	calendar_event get_midpoint(const calendar_event & e1,
								const calendar_event & e2);

	bool operator < (const calendar_event & e) {
		return date < e.date && time < e.time;
	}
};

calendar_event schedule_between(const calendar_event & e1,
								const calendar_event & e2,
								const int & d, bool * ok = NULL);
#endif
