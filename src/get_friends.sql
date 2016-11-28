DELIMITER $$
DROP PROCEDURE IF EXISTS GetFriends $$

CREATE PROCEDURE GetFriends(
 IN username VARCHAR(500),
 OUT success BOOLEAN)
BEGIN
 DECLARE uid INT DEFAULT 0;
 -- Find the user id
 SELECT user_id AS count INTO uid FROM users WHERE user_name = username;
 -- check that the user isn't in the group already
 SELECT DISTINCT u2.user_name FROM users u1, users u2, user_friend_relation
   WHERE u1.user_name = username AND
   ((user_friend_relation.user_id = u1.user_id AND
   user_friend_relation.friend_id = u2.user_id) OR
   (user_friend_relation.friend_id = u1.user_id AND
   user_friend_relation.user_id = u2.user_id)) AND
   u2.user_name != username AND
   user_friend_relation.accepted = 1;
 -- check they are nonzero
 IF uid = 0 THEN
  SET success = 0;
 ELSE
  SET success = 1;
 END IF;
END$$
DELIMITER ;
