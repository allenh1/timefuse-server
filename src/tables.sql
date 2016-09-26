CREATE TABLE IF NOT EXISTS schedules(
	schedule_id INTEGER NOT NULL AUTO_INCREMENT,
	PRIMARY KEY(schedule_id)
);

CREATE TABLE IF NOT EXISTS schedule_item(
	date DATE NOT NULL,
	start_time TIME NOT NULL,
	is_repeated BOOLEAN NOT NULL,
	duration INTEGER NOT NULL,
	location VARCHAR(512)  NOT NULL,
	event_name VARCHAR(512) NOT NULL,
	schedule_item_id INTEGER NOT NULL AUTO_INCREMENT,
	schedule_id INTEGER NOT NULL,
	PRIMARY KEY(schedule_item_id),
	FOREIGN KEY(schedule_id) REFERENCES schedules(schedule_id)
);


CREATE TABLE IF NOT EXISTS repeat_freq(
	mon BOOLEAN,
	tues BOOLEAN,
	wed  BOOLEAN,
	thurs BOOLEAN,
	fri BOOLEAN,
	sat BOOLEAN,
	sun BOOLEAN,
	weeks_per_rep INTEGER,
	month_per_rep INTEGER,
	year_per_rep INTEGER,
	schedule_item_id INTEGER NOT NULL,
	FOREIGN KEY(schedule_item_id) REFERENCES schedule_item(schedule_item_id)
);

CREATE TABLE IF NOT EXISTS users(
	user_id INTEGER NOT NULL AUTO_INCREMENT,
	schedule_id INTEGER NOT NULL,
	user_name VARCHAR(512) NOT NULL,
	passwd VARCHAR(512) NOT NULL,
	email VARCHAR(100) NOT NULL,
	cellphone BIGINT,
	PRIMARY KEY(user_id),
	FOREIGN KEY(schedule_id) REFERENCES schedules(schedule_id)
);


CREATE TABLE IF NOT EXISTS groups(
       group_id INTEGER NOT NULL AUTO_INCREMENT,
       schedule_id INTEGER NOT NULL,
       group_name VARCHAR(100) NOT NULL,
       PRIMARY KEY (group_id),
       FOREIGN KEY (schedule_id) REFERENCES schedules(schedule_id)
);

CREATE TABLE IF NOT EXISTS user_group_relation(
       user_id INTEGER NOT NULL,
       group_id INTEGER NOT NULL,
       FOREIGN KEY(user_id) REFERENCES users(user_id),
       FOREIGN KEY(group_id) REFERENCES groups(group_id)
);
