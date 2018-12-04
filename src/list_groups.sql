DELIMITER $$
DROP PROCEDURE IF EXISTS ListGroups$$

CREATE PROCEDURE ListGroups(
 IN user VARCHAR(500),
 IN pass VARCHAR(500),
 OUT success BOOLEAN)
BEGIN
 -- Count existing groups with that name.
 DECLARE X, id_no INT DEFAULT 0;
 SELECT count(group_name) AS count INTO X FROM groups WHERE group_name = groupName;
 -- Check that X is zero to avoid recreation.
 IF X != 0 THEN
  SET success = 0;
 ELSEIF X = 0 THEN
  -- create the schedule_id 
  INSERT INTO schedules(owner) VALUES(groupName);
  -- record the schedule_id		
  SELECT schedule_id INTO id_no FROM schedules WHERE owner = groupName;
  -- create the group
  INSERT INTO groups(schedule_id, group_name) VALUES(id_no, groupName);
  SET success = 1;
 END IF;
END$$
DELIMITER ;
