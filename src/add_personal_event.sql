DELIMITER $$
DROP PROCEDURE IF EXISTS AddPersonalEvent $$

CREATE PROCEDURE AddPersonalEvent(
 IN userName VARCHAR(100),
 IN eventDate DATE,
 IN startTime TIME,
 IN eventDuration INT,
 IN eventLocation VARCHAR(512),
 IN eventName VARCHAR(512), 
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
  -- insert the event
  INSERT INTO schedule_item(date, start_time, duration, location, event_name, schedule_id)
   VALUES(eventDate, startTime, eventDuration, eventLocation, eventName, sid);
  SET success = 1;
 END IF;
END$$
DELIMITER ;
