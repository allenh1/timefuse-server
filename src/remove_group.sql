DELIMITER $$
DROP PROCEDURE IF EXISTS RemoveGroup $$

CREATE PROCEDURE RemoveGroup(
 IN groupName VARCHAR(100),
 OUT success BOOLEAN)
BEGIN
 DECLARE gid, sid INT DEFAULT 0;
 -- Find the group id
 SELECT group_id AS count INTO gid FROM groups WHERE group_name = groupName;
 -- Check they are nonzero
 IF gid = 0 THEN
  SET success = 0;
 ELSE
  -- delete the group relations
  DELETE FROM user_group_relation WHERE group_id = gid;
  -- delete the group
  DELETE FROM groups WHERE group_id = gid;
  -- delete the schedule
  DELETE FROM schedules WHERE owner = groupName;
  SET success = 1;
 END IF;
END$$
DELIMITER ;
