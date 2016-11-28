DELIMITER $$
DROP PROCEDURE IF EXISTS DeleteFriend;

CREATE PROCEDURE DeleteFriend(
 IN username VARCHAR(100),
 IN friend VARCHAR(500),
 OUT success BOOLEAN)
BEGIN
 DECLARE fid, uid, X INT DEFAULT 0;
 -- Find the group id
 SELECT user_id AS count INTO uid FROM users WHERE user_name = username;
 -- Find the user id
 SELECT user_id AS count INTO fid FROM users WHERE user_name = friend;
 -- check that the user is in the group
 SELECT relation_id AS count INTO X FROM user_friend_relation
   WHERE (user_id = uid AND friend_id = fid) OR
   		 (user_id = fid AND friend_id = uid);
 -- Check they are nonzero
 IF fid = 0 or uid = 0 or X = 0 THEN
  SET success = 0;
 ELSE
  -- add an element to the user group relation.
  DELETE FROM user_friend_relation
  WHERE relation_id = X;		
  SET success = 1;
 END IF;
END$$
