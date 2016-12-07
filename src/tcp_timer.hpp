#ifndef __TCP_TIMER_HPP__
#define __TCP_TIMER_HPP__
#include <QtCore>
#include <iostream>

class tcp_timer : public QObject
{
	Q_OBJECT
public:
	explicit tcp_timer(size_t _seconds = 5,
					   QObject * _p_parent = NULL);
	virtual ~tcp_timer();

	bool init();

	Q_SIGNAL void timeout();
	Q_SLOT void run();
private:
	size_t m_seconds;
	QThread * m_p_thread;
};
#endif

