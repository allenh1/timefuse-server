DELIMITER $$
DROP PROCEDURE IF EXISTS AddPersonalEvent $$

CREATE PROCEDURE AddPersonalEvent(
 IN userName VARCHAR(500),
 IN eventDate DATE,
 IN startTime TIME,
 IN eventDuration TIME,
 IN eventLocation VARCHAR(512),
 IN eventOffset INT,
 IN eventName VARCHAR(512),
 IN immutable BOOLEAN,
 OUT success BOOLEAN)
BEGIN
 DECLARE sid, X INT DEFAULT 0;
 -- Find the schedule id
 SELECT schedule_id AS count INTO sid FROM schedules WHERE owner = userName;
 SELECT count(*) AS count INTO X FROM schedule_item WHERE location = eventLocation
 		AND schedule_id = sid AND start_time = startTime;
 -- Check they are nonzero
 IF sid = 0 OR X != 0 THEN
  SET success = 0;
 ELSE
  -- determine how to store things (i.e., is it mutable?)
  IF !immutable THEN
   -- interpret parameters differently.
   INSERT INTO schedule_item(date, start_time, duration, location,
    timezone_offset, event_name, schedule_id, immutable, deadline_date, deadline_time)
	VALUES(eventDate, SEC_TO_TIME(TIME_TO_SEC(startTime) - TIME_TO_SEC(eventDuration)),
	eventDuration, eventLocation, eventOffset, eventName, sid, immutable, eventDate, startTime);
  ELSE
   INSERT INTO schedule_item(date, start_time, duration, location,
    timezone_offset, event_name, schedule_id, immutable) VALUES(eventDate, startTime,
	eventDuration, eventLocation, eventOffset, eventName, sid, immutable);
  END IF;
  SET success = 1;
 END IF;
END$$
DELIMITER ;
