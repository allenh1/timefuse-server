DELIMITER $$
DROP PROCEDURE IF EXISTS RemoveFromGroup;

CREATE PROCEDURE RemoveFromGroup(
 IN groupName VARCHAR(100),
 IN userName VARCHAR(500),
 OUT success BOOLEAN)
BEGIN
 DECLARE gid, uid, X INT DEFAULT 0;
 -- Find the group id
 SELECT group_id AS count INTO gid FROM groups WHERE group_name = groupName;
 -- Find the user id
 SELECT user_id AS count INTO uid FROM users WHERE user_name = userName;
 -- check that the user isn't in the group already
 SELECT count(user_id) AS count INTO X FROM user_group_relation
   WHERE user_id = uid AND group_id = gid;
 -- Check they are nonzero
 IF gid = 0 or uid = 0 THEN
  SET success = 0;
 ELSEIF X = 0 THEN
  SET success = 0;
 ELSE
  -- add an element to the user group relation.
  DELETE FROM user_group_relation WHERE user_id = uid AND group_id = gid; 
  SET success = 1;
 END IF;
END$$
