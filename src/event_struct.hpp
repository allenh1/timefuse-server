// Copyright 2017 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __EVENT_STRUCT_HPP__
#define __EVENT_STRUCT_HPP__
#include <QDate>
#include <QTime>
#include <QSet>

struct calendar_event
{
  QDate date;
  QTime time;
  int duration;   /* in minutes */

  calendar_event get_midpoint(
    const calendar_event & e1,
    const calendar_event & e2);

  QTime get_finish_time()
  {
    QTime ret = time;
    return ret.addSecs(duration * 60);
  }

  bool operator<(const calendar_event & e)
  {
    return date < e.date && time < e.time;
  }

  bool operator==(const calendar_event & e)
  {
    return date == e.date && time == e.time;
  }
};

calendar_event schedule_between(
  const QList<calendar_event> & events,
  const size_t & index_1,
  const size_t & index_2,
  const int & d, bool * ok);

bool check_overlap(
  calendar_event e1,
  calendar_event e2);

/**
 * Equal to operator for calendar events.
 *
 * Was QSet really worth it?
 *
 * @param e1 first event
 * @param e2 second event
 *
 * @return True if the two events start at the same time.
 */
inline bool operator==(
  const calendar_event & e1,
  const calendar_event & e2)
{return e1.date == e2.date && e1.time == e2.time;}

/**
 * Overloaded qHash function for a calendar_event.
 *
 * @param e Event to hash.
 * @param seed Idk. Maybe it's gonna be a tree.
 *
 * @return Whatever qHash usually returns?
 */
inline uint qHash(const calendar_event & e, uint seed)
{
  return qHash(e.date.toString("yyyy-M-d"), seed) ^
         qHash(e.time.toString("hh:mm"), seed) ^
         qHash(e.duration, seed);
}
#endif
