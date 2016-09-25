TimeFuse-Server
===============
This repository holds the server and the server worker for the TimeFuse
project!

TimeFuse-Server
---------------
This is the master. There should be one instance of this per instance
of a database. Though, unless someone else is mirroring this project,
there will only be one instance of these running at a time.

The master is responsible for connecting users to individual worker nodes.

TimeFuse-Worker
----------------
This is the real power-horse. The TimeFuse-Worker connects to master
and awaits a redirection to a user. Then the user's request is served.

The TimeFuse-Worker is always "connected" to the IRCServer and the MySQL
database.

