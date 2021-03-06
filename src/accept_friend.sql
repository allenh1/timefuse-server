DELIMITER $$
DROP PROCEDURE IF EXISTS AcceptFriend $$

CREATE PROCEDURE AcceptFriend(
 IN username VARCHAR(500),
 IN friend VARCHAR(500), 
 OUT success BOOLEAN)
BEGIN
 DECLARE uid, fid, X INT DEFAULT 0;
 -- Find the user id
 SELECT user_id AS count INTO uid FROM users WHERE user_name = username;
 -- Find the friend id
 SELECT user_id AS count INTO fid FROM users WHERE user_name = friend;
 -- check that the user isn't in the group already
 SELECT relation_id AS count INTO X FROM user_friend_relation
   WHERE (user_id = uid AND friend_id = fid)
   OR (user_id = fid AND friend_id = uid);
 -- check they are nonzero
 IF fid = 0 or uid = 0 or X = 0 THEN
  SET success = 0;
 ELSE
  -- add an element to the user friend relation.
  UPDATE user_friend_relation SET accepted=1
  WHERE relation_id = X;
  SET success = 1;
 END IF;
END$$
DELIMITER ;
